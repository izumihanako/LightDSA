#include "src/async_dsa.hpp" 
#include "src/details/dsa_cpupath.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib> 
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
using namespace std ;

double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;
constexpr int REPEAT = 10 ;  
size_t array_len = 4 * GB ;
int desc_cnt = 100000 ;
char* src_arr = nullptr , *dest_arr = nullptr ;

struct OffLen{
    size_t off_src ;
    size_t off_dest ;
    size_t len ;
    OffLen( size_t off_src_ , size_t off_dest_ , size_t len_ ) : off_src( off_src_ ) , off_dest( off_dest_ ) , len( len_ ) {} 
} ;

string stdsiz( size_t siz ) {
    double regsiz = siz ;
    int regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    char rt[20] ;
    regsiz = floor( regsiz * 1000 + 0.5 ) / 1000 ;
    sprintf( rt , "%.4g%2s" , regsiz , regstr[regcnt] ) ;
    return string( rt ) ;
}

vector<OffLen> genTestset( int desc_cnt , size_t array_len , size_t transfer_size , int access_type ){
    vector<OffLen> rt ;
    size_t offset = ( rand() % array_len ) & (~0xfff) ; // random start point to avoid CPU cache
    for( int i = 0 ; i < desc_cnt ; i ++ ){
        size_t len = transfer_size ;
        if( offset + len > array_len ) offset = 0 ;
        if( access_type == 0 ){ // seq 
            rt.push_back( OffLen( offset , offset , len ) ) ;
        } else { // random 
            size_t src_off = ( rand() % ( array_len - len ) ) & (~0xfff) ;
            size_t dest_off = offset ;
            rt.push_back( OffLen( src_off , dest_off , len ) ) ;
        }
        offset += len ;
    }
    return rt ;
}

DSAbatch dsa_batch( 32 , 80 ) ; 
uint64_t pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_ & 0xff ;
void test_dsa_speed( int access_type , int method , int aligned ){
    char* _src = (char*) aligned_alloc( 4096 , array_len ) ;
    char* _dest = (char*) aligned_alloc( 4096 , array_len ) ;
    if( aligned ){
        src_arr = _src ;
        dest_arr = _dest ; 
    } else {
        src_arr = _src + 0x555 ;
        dest_arr = _dest + 0x555 ;
    }
 
    printf( "method = %s , op = memcpy, array_len = %s, access_type = %s, aligned = %d\n" ,
            method == 0 ? "CPU" : "DSA" , stdsiz( array_len ).c_str() ,
            access_type == 0 ? "sequential" : "random" , aligned ) ;
    fflush( stdout ) ;

    for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i , dest_arr[i] = i % 256 ;  

    for( size_t transfer_size = 256 ; transfer_size <= 128 * KB ; transfer_size *= 2 ){ 
        double do_time = 0 , do_speed = 0 ;
        for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){
            vector<OffLen> test_set = genTestset( desc_cnt , array_len , transfer_size , access_type ) ;
            size_t this_xfersize = 0 ;
            for( auto& it : test_set ) this_xfersize += it.len ;

            for( size_t i = 0 ; i < array_len ; i += 4096 ) {
                src_arr[i] = src_arr[i] ^ 0;
                dest_arr[i] = dest_arr[i] ^ 0 ;
            }
            uint64_t start = timeStamp_hires() ;
            // DSA_batch
            if( method == 1 ){
                for( auto& it : test_set ){
                    dsa_batch.submit_memmove( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ; 
                }
                dsa_batch.wait() ;
            } else { 
                for( int i = 0 , lim = test_set.size() ; i < lim ; i ++ ){
                    auto& it = test_set[i] ;
                    if( i < lim - 1 ) __builtin_prefetch( src_arr + test_set[i+1].off_src , 1 , 0 ) ; 
                    memmove_cpu( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ; 
                } 
            } 
            if( warmup <= 0 ){
                warmup ++ ; 
                repeat -- ;
                continue ;
            }
            uint64_t end = timeStamp_hires() ; 
            do_time += ( end - start ) / 1.0 / REPEAT;
        } 
        do_time = do_time * ns_to_us / desc_cnt ; // us 
        do_speed = transfer_size / ( do_time * us_to_s ) / MB ;
        printf( "desc_cnt = %5d | transfer_size = %7s | avg_time = %8.2f us | do_speed = %5.0f MB/s | REPEAT = %d\n" , 
                desc_cnt , stdsiz( transfer_size ).c_str() , do_time / REPEAT , do_speed , REPEAT ) ; fflush( stdout ) ;  
    }
    if( method == 1 ) dsa_batch.print_stats() ;
    free( _src ) ;
    free( _dest ) ;
} 
 
DSAop ___ ;
int main( int argc , char** argv ){
    srand( 19260817 ) ; 
    int access_type = 0 , method = 0 ;
    if( argc > 2 ) {
        access_type = atoi( argv[1] ) ;
        method = atoi( argv[2] ) ; 
        for( int aligned = 0 ; aligned <= 0 ; aligned ++ ){
            test_dsa_speed( access_type , method , aligned ) ;
            puts( "" ) ; 
        } 
    } else {
        printf( "Usage: %s <access_type> <method>\n" , argv[0] ) ;
        printf( "access_type : 0 is sequential, 1 is random\n" ) ;
        printf( "method      : 0 is CPU memcpy, 1 is DSA_batch\n" ) ;
    }
    return 0 ;
}