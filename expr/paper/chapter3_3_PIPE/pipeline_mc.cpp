#include "src/lightdsa.hpp" 

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
constexpr int bsiz = 32 , BIG_LEN = 16 * KB , SMALL_LEN = 512 ; 
int small_cnt = 1 , big_cnt = 1 , desc_cnt = 100 , method = 0 , mix_len ;

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

void test_dsa_batch( int small_cnt , int big_cnt , vector<OffLen> test_set , int method ){ 
    double st_time , ed_time , do_time ;
    int cnt = test_set.size() ;

    // random_shuffle( test_set.begin() , test_set.end() ) ; 
    // for( int i = 0 ; i < cnt ; i ++ ) swap( test_set[i].off_dest , test_set[ rand()%cnt ].off_dest ) ;

    size_t tot_siz = 0 ;
    for( int i = 0 ; i < cnt ; i ++ ) tot_siz += test_set[i].len ; 
    printf( "Copy %s; %d desc (%s:%s = %d:%d); REPEAT = %d \n" , stdsiz( tot_siz ).c_str() , cnt ,
             stdsiz( SMALL_LEN ).c_str() , stdsiz( BIG_LEN ).c_str() , small_cnt , big_cnt , REPEAT ) ;

    DSAbatch xfer( bsiz , 20 ) ; 
    DSAop memcpys[bsiz] ; 
    double dsa_time = 0 , dsa_speed = 0 , desc_speed = 0 ;
    char *a = (char*) aligned_alloc( 4096 , tot_siz ) , *b = (char*) aligned_alloc( 4096 , tot_siz ) ;  
    touch_pf( a , tot_siz ) ; touch_pf( b , tot_siz ) ; 
    printf( "a @ %p , b @ %p\n" , a , b ) ;
    if( method == 0 ){ 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
            xfer.db_task.clear() ; 
            st_time = timeStamp_hires() ;
            for( int i = 0 ; i < cnt ; i ++ ) xfer.submit_memmove( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ) ; 
            xfer.db_task.wait() ;
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            dsa_time += ( do_time ) / REPEAT ;
        }
    } else if( method == 1 ){
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){  
            st_time = timeStamp_hires() ;  
            for( int i = 0 ; i < cnt ; i ++ ){
                int id = i % bsiz ;
                memcpys[id].wait() ;
                memcpys[id].async_memmove( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ) ;
            }
            for( int i = 0 ; i < bsiz ; i ++ ) memcpys[i].wait() ; 
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            dsa_time += ( do_time ) / REPEAT ;
        }
    }
    dsa_time *= ns_to_us ; // us
    dsa_speed = tot_siz / ( dsa_time * us_to_s ) / MB ; 
    desc_speed = desc_cnt / dsa_time ;  
    if( method == 0 ) printf( "    ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s, %.2fdesc/us\n" , dsa_time , dsa_speed , desc_speed ) ; 
    if( method == 1 ) printf( "    ; DSA_memcpy cost %5.2lfus, speed %5.0fMB/s, %.2fdesc/us\n" , dsa_time , dsa_speed , desc_speed ) ; 
    free( a ) ; free( b ) ;
    if( method == 0 ) xfer.print_stats() ;
} 
 
vector<OffLen> genSeparate( int small_cnt , int big_cnt , int desc_cnt , int mix_len ){
    vector<OffLen> rt , big , small ;
    size_t offset = 0 ;
    int in_seg_big = mix_len / ( big_cnt + small_cnt ) * big_cnt ;
    int in_seg_small = mix_len / ( big_cnt + small_cnt ) * small_cnt ;

    for( int i = 0 ; i < desc_cnt ; ){
        for( int j = 0 ; j < in_seg_big && i < desc_cnt ; j ++ , i ++ ){ 
            big.emplace_back( offset , offset , BIG_LEN ) ;
            offset += BIG_LEN ; 
        }
        for( int j = 0 ; j < in_seg_small && i < desc_cnt ; j ++ , i ++ ){ 
            small.emplace_back( offset , offset , SMALL_LEN ) ;
            offset += SMALL_LEN ; 
        }
    } 
    random_shuffle( small.begin() , small.end() ) ;
    random_shuffle( big.begin() , big.end() ) ;
    int big_idx = 0 , small_idx = 0 ;
    for( int i = 0 ; i < desc_cnt ; ){
        for( int j = 0 ; j < in_seg_big && i < desc_cnt ; j ++ , i ++ ){
            rt.push_back( big[big_idx++] ) ;  
        }
        for( int j = 0 ; j < in_seg_small && i < desc_cnt ; j ++ , i ++ ){  
            rt.push_back( small[small_idx++] ) ; 
        }
    }
    return rt ;
}

DSAop ___ ;
int main( int argc , char** argv ){ 
    if( argc < 6 ){ 
        printf("Usage: %s <method> <desc_cnt> <small_cnt> <big_cnt> <mix_len>\n", argv[0]) ;
        printf( "method    : 0 is DSA_batch, 1 is DSA_memcpy\n" ) ;
        printf( "desc_cnt  : number of descs\n" ) ;
        printf( "small_cnt : small desc in a group\n" ) ;
        printf( "big_cnt   : big desc in a group\n" ) ;
        printf( "mix_len   : smallest segment to mix big/small memcpy\n" ) ;
        return 0 ;
    } else {
        method = atoi( argv[1] ) ; 
        desc_cnt = atoi( argv[2] ) ;
        small_cnt = atoi( argv[3] ) ; 
        big_cnt = atoi( argv[4] ) ; 
        mix_len = atoi( argv[5] ) ;
        if( mix_len % ( small_cnt + big_cnt ) != 0 ){
            printf( "mix_len should be divisible by (small_cnt + big_cnt)\n" ) ;
            return 0 ;
        }
    }  
    printf( "method = %s , desc_cnt = %d (small(%s):big(%s) = %d:%d), mix_len = %d\n" ,
            method == 0 ? "DSA_batch" : "DSA_memcpy" , desc_cnt , 
            stdsiz(SMALL_LEN).c_str(), stdsiz(BIG_LEN).c_str() , small_cnt , big_cnt , mix_len ) ;  
    vector<OffLen> test_set = genSeparate( small_cnt , big_cnt , desc_cnt , mix_len ) ;
     
    printf( "[ " ) ;
    for( int i = 0 , cnt = 0 , type = 0 , switc = 0 ; (size_t)i < test_set.size() && switc < 8 ; i ++ ){ 
        if( test_set[i].len == BIG_LEN ){
            if( type == 0 ) cnt ++ ;
            if( type == 1 ) {
                printf_RGB( 0xcccc00 , "S%d " , cnt ) ;
                type = 0 , cnt = 1 , switc ++ ;
            }
        } else {
            if( type == 1 ) cnt ++ ;
            if( type == 0 ) {
                printf_RGB( 0x00cc00 , "B%d " , cnt ) ;
                type = 1 , cnt = 1 , switc ++ ;
            }
        }
    } printf( "... (%ld descs) ] \n" , test_set.size() ) ;

    test_dsa_batch( small_cnt , big_cnt , test_set , method ) ;
    return 0 ;
}

// ./pipeline_mc 0 100000 1 1 (2*x) where x means mix-granularity