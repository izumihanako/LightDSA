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
size_t array_len = 4 * GB , range_lower = 128 , range_upper = 16 * KB ;
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

vector<OffLen> genTestset( int desc_cnt , size_t array_len , size_t range_L , size_t range_R , int access_type ){
    vector<OffLen> rt ;
    size_t offset = rand() % array_len ; // random start point to avoid CPU cache
    for( int i = 0 ; i < desc_cnt ; i ++ ){
        size_t len = range_L + ( rand() % ( range_R - range_L + 1 ) ) ;
        if( offset + len > array_len ) offset = 0 ;
        if( access_type == 0 ){ // seq 
            rt.push_back( OffLen( offset , offset , len ) ) ;
        } else { // random 
            size_t src_off = rand() % ( array_len - len ) ;
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
void test_dsa_speed( int access_type , int method ){ 
    char* _src = (char*) aligned_alloc( 4096 , array_len ) ;
    char* _dest = (char*) aligned_alloc( 4096 , array_len ) ;
    src_arr = _src ; dest_arr = _dest ; 
 
    printf( "method = %s , op = memcpy, array_len = %s, access_type = %s, range in [%s, %s]\n" ,
            method == 0 ? "CPU" : "DSA" , stdsiz( array_len ).c_str() ,
            access_type == 0 ? "sequential" : "random" ,
            stdsiz( range_lower ).c_str() , stdsiz( range_upper ).c_str() ) ;
    fflush( stdout ) ;

    for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i , dest_arr[i] = i % 256 ;  

    for( int desc_cnt = 250 ; desc_cnt <= 131072 ; desc_cnt *= 2 ){
        size_t tot_xfersize = 0 ; 
        double do_time = 0 , do_speed = 0 ;
        for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){
            vector<OffLen> test_set = genTestset( desc_cnt , array_len , range_lower , range_upper , access_type ) ;
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
                    dsa_batch.submit_memcpy( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ; 
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
            tot_xfersize += this_xfersize ;
            do_time += ( end - start ) ;
        } 
        do_time *= ns_to_us ; // us 
        do_speed = tot_xfersize / ( do_time * us_to_s ) / MB ;
        printf( "desc_cnt = %5d , avg_size = %7s | avg_time = %8.2f us | do_speed = %5.0f MB/s | REPEAT = %d\n" , 
                desc_cnt , stdsiz( tot_xfersize / REPEAT ).c_str() , do_time / REPEAT , do_speed , REPEAT ) ; fflush( stdout ) ;  
    }
    if( method == 1 ) dsa_batch.print_stats() ;
    free( _src ) ;
    free( _dest ) ;
} 
 
DSAop ___ ;
int main(){
    srand( 19260817 ) ;
    for( size_t upper_size = 8 * KB ; upper_size <= 8 * KB ; upper_size *= 2 ){ 
        range_upper = upper_size ;
        for( int access_type = 0 ; access_type <= 1 ; access_type ++ ){
            for( int method = 0 ; method <= 1 ; method ++ ){ 
                test_dsa_speed( access_type , method ) ;
                puts( "" ) ; 
            }
        }
    }
    return 0 ;
}