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
    size_t offset = 0 ;
    for( int i = 0 ; i < desc_cnt ; i ++ ){
        if( offset + transfer_size > array_len ) offset = 0 ;
        rt.push_back( OffLen( offset , offset , transfer_size ) );
        offset += transfer_size ;
    }
    if( access_type == 1 ) random_shuffle( rt.begin() , rt.end() ) ;
    return rt ;
}

uint64_t pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_ & 0xff ;
void test_dsa_speed( int op_type , int access_type ){ 
    char* _src = (char*) aligned_alloc( 4096 , array_len ) ;
    char* _dest = (char*) aligned_alloc( 4096 , array_len ) ;
    src_arr = _src ; dest_arr = _dest ; 

    printf( "method = batch32, op = %s, array_len = %s, access_type = %s\n" ,
            op_type == 0 ? "memcpy" : op_type == 1 ? "memfill" : op_type == 2 ? "compare" : "compval" ,
            stdsiz( array_len ).c_str() , access_type == 0 ? "sequential" : "random" ) ; fflush( stdout ) ;

    if( op_type == 0 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i , dest_arr[i] = 0 ; 
    } else if( op_type == 1 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = 0 ;
    }else if( op_type == 2 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = i ;
    } else if( op_type == 3 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ;
    }

    DSAbatch dsa_batch( 32 , 80 ) ; 
    for( size_t transfer_size = 128 ; transfer_size <= 8 * KB ; transfer_size *= 2 ){
        size_t desc_cnt = array_len / transfer_size ;
        vector<OffLen> test_set = genTestset( desc_cnt , array_len , transfer_size , access_type ) ;
        size_t tot_xfersize = 0 ;
        for( auto& it : test_set ) tot_xfersize += it.len ;

        double do_time = 0 , do_speed = 0 ;
        for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){
            for( size_t i = 0 ; i < array_len ; i += 4096 ) {
                src_arr[i] = src_arr[i] ^ 0;
                dest_arr[i] = dest_arr[i] ^ 0 ;
            }
            uint64_t start = timeStamp_hires() ;
            // DSA_batch
            for( auto& it : test_set ){
                if( op_type == 0 ) dsa_batch.submit_memmove( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                else if( op_type == 1 ) dsa_batch.submit_memfill( dest_arr + it.off_dest , pattern_ , it.len ) ;
                else if( op_type == 2 ) dsa_batch.submit_compare( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                else if( op_type == 3 ) dsa_batch.submit_comp_pattern( src_arr + it.off_src , pattern_ , it.len ) ;
            }
            dsa_batch.wait() ;
            if( warmup <= 0 ){
                warmup ++ ; 
                repeat -- ;
                continue ;
            }
            uint64_t end = timeStamp_hires() ;
            do_time += ( end - start ) / REPEAT ;
        }
        if( op_type == 0 ) {
            for( int i = 0 , lim = min( tot_xfersize , array_len ) ; i < lim ; i ++ )
                if( src_arr[i] != dest_arr[i] ) {
                    printf( "Error: src[%d] = %d , dest[%d] = %d\n" , i , src_arr[i] , i , dest_arr[i] ) ;
                    break ;
                }
        } else if( op_type == 1 ){
            for( int i = 0 , lim = min( tot_xfersize , array_len ) ; i < lim ; i ++ )
                if( char_patt != dest_arr[i] ) {
                    printf( "Error: src[%d] = %d , dest[%d] = %d\n" , i , src_arr[i] , i , dest_arr[i] ) ;
                    break ;
                }
        }
        do_time *= ns_to_us ; // us
        do_time /= desc_cnt ; // single desc time
        do_speed = transfer_size / ( do_time * us_to_s ) / MB ; // GB/s
        printf( "transfer_size = %6s | do_time = %5.2f us | do_speed = %5.0f MB/s | REPEAT = %d\n" , 
                stdsiz( transfer_size ).c_str() , do_time , do_speed , REPEAT ) ; fflush( stdout ) ; 
    }
    dsa_batch.print_stats() ;

    // test speed for single core CPU 
    for( size_t transfer_size = 128 ; transfer_size <= 8 * KB ; transfer_size *= 2 ){ 
        size_t desc_cnt = array_len / transfer_size ;
        vector<OffLen> test_set = genTestset( desc_cnt , array_len , transfer_size , access_type ) ;
        double do_time = 0 , do_speed = 0 ;
        for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){
            for( size_t i = 0 ; i < array_len ; i += 4096 ) {
                src_arr[i] = src_arr[i] ^ 0;
                dest_arr[i] = dest_arr[i] ^ 0 ;
            }
            uint64_t start = timeStamp_hires() ; 
            bool res = false ;
            for( auto& it : test_set ){
                if( op_type == 0 ) memmove_cpu( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                else if( op_type == 1 ) memfill_cpu( dest_arr + it.off_dest , pattern_ , it.len ) ;
                else if( op_type == 2 ) res |= ( compare_cpu( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) != (int)it.len ) ;
                else if( op_type == 3 ) res |= ( compval_cpu( src_arr + it.off_src , pattern_ , it.len ) != (int)it.len ) ;
            }
            if( res ) puts( "Error: compare result error" ) ;
            if( warmup <= 0 ){
                warmup ++ ; 
                repeat -- ;
                continue ;
            }
            uint64_t end = timeStamp_hires() ;
            do_time += ( end - start ) / REPEAT ;
        } 
        do_time *= ns_to_us ; // us
        do_time /= desc_cnt ; // single desc time
        do_speed = transfer_size / ( do_time * us_to_s ) / MB ; // GB/s
        printf( "CPU: transfer_size = %6s | do_time = %5.2f us | do_speed = %5.0f MB/s | REPEAT = %d\n" , 
                stdsiz( transfer_size ).c_str() , do_time , do_speed , REPEAT ) ; fflush( stdout ) ; 
    } 
    free( _src ) ;
    free( _dest ) ;
} 
 
DSAop ___ ;
int main( int argc , char** argv ){
    int op = 0 , access_type = 0 ;
    if( argc >= 3 ){
        op = atoi( argv[1] ) ;
        access_type = atoi( argv[2] ) ;
        if( op >= 0 && op <= 3 ) test_dsa_speed( op , access_type ) ; 
        else {
            puts( "Invalid op type" ) ;
            return -1 ;
        }
    }
    else {
        puts( "Usage: ./DSA_speed <op> <access_type>" ) ;
        puts( "  op : 0 is memmove , 1 is memfill , 2 is compare , 3 is compval" ) ;
        puts( "  access_type : 0 is sequential , 1 is random" ) ; 
    }
    return 0 ;
}