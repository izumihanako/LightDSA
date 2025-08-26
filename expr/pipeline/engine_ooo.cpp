#include "src/async_dsa.hpp" 

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
constexpr int COPY_LEN = 1 * KB ; 
int desc_cnt = 1 , method = 0 , is_random = 0 , wq_cnt = 4 ;
size_t array_len , stride ;

struct OffLen{
    size_t off_src ;
    size_t off_dest ;
    size_t len ;
    OffLen( size_t off_src_ , size_t off_dest_ , size_t len_ ) : off_src( off_src_ ) , off_dest( off_dest_ ) , len( len_ ) {} 
} ;

inline void touch_pf( char* x , size_t len ){ 
    if( len ) *(x+len-1) = 'A' ; 
    for(char *uplim = x + len ; x < uplim ; x += 4096 ) *x = 'A' ; 
}
 
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

void test_dsa_batch( size_t array_len , int cnt , vector<OffLen> test_set , int method ){ 
    double st_time , ed_time , do_time ;
    double dsa_time = 0 , dsa_speed = 0 , desc_speed = 0 ;

    if( is_random ) random_shuffle( test_set.begin() , test_set.end() ) ;  

    size_t tot_siz = 0 ;
    for( int i = 0 ; i < cnt ; i ++ ) tot_siz += test_set[i].len ; 
    char *a = (char*) aligned_alloc( 4096 , array_len ) , *b = (char*) aligned_alloc( 4096 , array_len ) ;
    touch_pf( a , array_len ) ; touch_pf( b , array_len ) ; 
    printf( "a @ %p , b @ %p\n" , a , b ) ;

    if( method == 0 ){
        DSAbatch xfer[wq_cnt] ;
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
            invld_range( a , array_len ) ;
            invld_range( b , array_len ) ;
            for (int j = 0; j < wq_cnt ; j++) { xfer[j].db_task.clear(); }
            st_time = timeStamp_hires() ;
            for( int i = 0 ; i < cnt ; i ++ )
                xfer[i % wq_cnt].submit_memmove( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ); 
            for (int j = 0; j < wq_cnt; j++)xfer[j].db_task.wait(); 
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            dsa_time += ( do_time ) / REPEAT ;
        }
    } else if( method == 1 ){
        DSAop memcpys[wq_cnt] ; 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){  
            invld_range( a , array_len ) ;
            invld_range( b , array_len ) ;
            st_time = timeStamp_hires() ;  
            for( int i = 0 ; i < cnt ; i ++ ){ 
                int id = i % wq_cnt ;
                memcpys[id].wait() ;
                memcpys[id].async_memmove( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ) ;
            }
            for( int i = 0 ; i < wq_cnt ; i ++ ) memcpys[i].wait() ; 
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            dsa_time += ( do_time ) / REPEAT ;
        }
    }
    free( a ) ; free( b ) ;
    dsa_time *= ns_to_us ; // us
    dsa_speed = tot_siz / ( dsa_time * us_to_s ) / MB ; 
    desc_speed = cnt / dsa_time ;

    printf( "Copy %s; %d desc * %s , array = %s; REPEAT = %d \n" , stdsiz( tot_siz ).c_str() , cnt ,
             stdsiz( COPY_LEN ).c_str() , stdsiz( array_len ).c_str() , REPEAT ) ;
    if( method == 0 ) printf( "    ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s, %.2fdesc/us\n" , dsa_time , dsa_speed , desc_speed ) ;
    if( method == 1 ) printf( "    ; DSA_memcpy cost %5.2lfus, speed %5.0fMB/s, %.2fdesc/us\n" , dsa_time , dsa_speed , desc_speed ) ;
} 
 
vector<OffLen> genSeparate( uint64_t len , int desc_cnt , uint64_t stride ) {
    vector<OffLen> rt ; 
    size_t offset = 0 ;
    for( int i = 1 ; i <= desc_cnt ; i ++ ){
        rt.emplace_back( offset , offset , COPY_LEN ) ;
        offset += stride ;
        if( offset + COPY_LEN >= len ) offset = 0 ;
    }
    return rt;
}

DSAop ___ ;
int main( int argc , char** argv ){ 
    if( argc < 7 ){ 
        printf("Usage: %s <method> <wq_cnt> <desc_cnt> <stride> <array_len> <is_random>\n", argv[0]) ;
        printf( "method    : 0 is DSA_batch, 1 is DSA_memcpy\n" ) ;
        printf( "desc_cnt  : number of descs\n" ) ;
        printf( "stride(KB): diff between nearest descs\n" ) ;
        printf( "array_len(MB) : length of the array\n" ) ;
        printf( "is_random : 1 is random, 0 is not\n" ) ;
        return 0 ;
    } else {
        method = atoi( argv[1] ) ; 
        wq_cnt = atoi( argv[2] ) ;
        desc_cnt = atoi( argv[3] ) ;
        stride = atoi( argv[4] ) * KB ; 
        array_len = atoi( argv[5] ) * MB ; 
        is_random = atoi( argv[6] ) ;
    } 
    // method = 0 , desc_cnt = 4096 , stride = 512 * KB , array_len = 4096 * MB , is_random = 0 ;
    printf( "method = %s,wq_cnt = %d, desc_cnt = %d * %s, stride = %s, array_len = %s, %s\n" ,
            method == 0 ? "DSA_batch" : "DSA_memcpy" , wq_cnt , desc_cnt , stdsiz(COPY_LEN).c_str() 
            , stdsiz( stride ).c_str() , stdsiz( array_len ).c_str() , is_random == 0 ? "seq" : "random" ) ;
    fflush( stdout ) ;

    vector<OffLen> test_set = genSeparate( array_len , desc_cnt , stride ) ;
    test_dsa_batch( array_len , desc_cnt , test_set , method ) ;
    return 0 ;
}
/**
./setup_dsa.sh -d dsa0 -g0 -w1 -q0 -s32 -ms -e1 -b0
./setup_dsa.sh -d dsa0 -g1 -w1 -q1 -s32 -ms -e1 -b0
./setup_dsa.sh -d dsa0 -g2 -w1 -q2 -s32 -ms -e1 -b0
./setup_dsa.sh -d dsa0 -g3 -w1 -q3 -s32 -ms -e1 -b0
./setup_dsa.sh -d dsa0 -b1
*/
// echo 24 > /sys/bus/dsa/devices/dsa0/group0.*/read_buffers_allowed
// echo 24 > /sys/bus/dsa/devices/dsa0/group0.*/read_buffers_reserved
// echo 1 > /sys/bus/dsa/devices/dsa0/group0.*/use_read_buffer_limit