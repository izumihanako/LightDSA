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
constexpr int bsiz = 64 , BIG_LEN = 16 * KB , SMALL_LEN = 512 ; 
int small_cnt = 1 , big_cnt = 1 , group_cnt = 100 , method = 0 , is_divide = 0 ;

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
    double dsa_time = 0 , dsa_speed = 0 , big_speed , small_speed ;
    int cnt = test_set.size() ;

    // random_shuffle( test_set.begin() , test_set.end() ) ; 
    // for( int i = 0 ; i < cnt ; i ++ ) swap( test_set[i].off_dest , test_set[ rand()%cnt ].off_dest ) ;

    size_t tot_siz = 0 ;
    for( int i = 0 ; i < cnt ; i ++ ) tot_siz += test_set[i].len ; 
    char *a = (char*) aligned_alloc( 4096 , tot_siz * 2 ) , *b = (char*) aligned_alloc( 4096 , tot_siz * 2 ) ;  
    touch_pf( a , tot_siz * 2 ) ; touch_pf( b , tot_siz * 2 ) ; 
    printf( "a @ %p , b @ %p\n" , a , b ) ;

    if( method == 0 ){ 
        DSAbatch xfer( bsiz , DEFAULT_BATCH_CAPACITY ) ; 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
            xfer.db_task.clear() ; 
            st_time = timeStamp_hires() ;
            for( int i = 0 ; i < cnt ; i ++ ) xfer.submit_memcpy( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ) ; 
            xfer.db_task.wait() ;
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            dsa_time += ( do_time ) / REPEAT ;
        }
    } else if( method == 1 ){
        DSAmemcpy memcpys[bsiz] ; 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){  
            st_time = timeStamp_hires() ;  
            for( int i = 0 ; i < cnt ; i ++ ){
                int id = i % bsiz ;
                memcpys[id].wait() ;
                memcpys[id].do_async( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ) ;
            }
            for( int i = 0 ; i < bsiz ; i ++ ) memcpys[i].wait() ; 
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            dsa_time += ( do_time ) / REPEAT ;
        }
    }
    free( a ) ; free( b ) ;
    dsa_time *= ns_to_us ; // us
    dsa_speed = tot_siz / ( dsa_time * us_to_s ) / MB ; 
    small_speed = small_cnt * group_cnt / dsa_time ;
    big_speed   = big_cnt * group_cnt / dsa_time ;

    printf( "Copy %s; %d desc (%s:%s = %d:%d); REPEAT = %d \n" , stdsiz( tot_siz ).c_str() , cnt ,
             stdsiz( SMALL_LEN ).c_str() , stdsiz( BIG_LEN ).c_str() , small_cnt , big_cnt , REPEAT ) ;
    if( method == 0 ) printf( "    ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s, big %.2fdesc/us, small %.2fdesc/us\n" , dsa_time , dsa_speed , big_speed , small_speed ) ;
    if( method == 1 ) printf( "    ; DSA_memcpy cost %5.2lfus, speed %5.0fMB/s, big %.2fdesc/us, small %.2fdesc/us\n" , dsa_time , dsa_speed , big_speed , small_speed ) ;
} 
 
vector<OffLen> genSeparate( int small_cnt , int big_cnt , int group_cnt , int is_divide ) {
    vector<OffLen> rt ;
    size_t offset = 0 ;
    if( is_divide ){
        for( int g = 0 ; g < group_cnt ; g ++ ){
            for( int s = 1 ; s <= small_cnt ; s ++ ) {
                rt.emplace_back( offset , offset , SMALL_LEN ) ;
                offset += SMALL_LEN ;
            }
        }
        for( int g = 0 ; g < group_cnt ; g ++ ){
            for( int b = 1 ; b <= big_cnt ; b ++ ){
                rt.emplace_back( offset , offset , BIG_LEN ) ;
                offset += BIG_LEN ;
            }
        }
    } else {
        for( int g = 0 ; g < group_cnt ; g ++ ){
            for( int s = 1 ; s <= small_cnt ; s ++ ) {
                rt.emplace_back( offset , offset , SMALL_LEN ) ;
                offset += SMALL_LEN ;
            }
            for( int b = 1 ; b <= big_cnt ; b ++ ){
                rt.emplace_back( offset , offset , BIG_LEN ) ;
                offset += BIG_LEN ;
            }
        }
    }
    return rt ;
}

DSAmemcpy ___ ;
int main( int argc , char** argv ){ 
    if( argc < 6 ){ 
        printf("Usage: %s <method> <group_cnt> <small_cnt> <big_cnt> <is_divide>\n", argv[0]) ;
        printf( "method    : 0 is DSA_batch, 1 is DSA_memcpy\n" ) ;
        printf( "group_cnt : number of groups\n" ) ;
        printf( "small_cnt : small desc in a group\n" ) ;
        printf( "big_cnt   : big desc in a group\n" ) ;
        printf( "is_divide : divide small/big descs, 0 no, 1 yes\n" ) ;
        return 0 ;
    } else {
        method = atoi( argv[1] ) ; 
        group_cnt = atoi( argv[2] ) ;
        small_cnt = atoi( argv[3] ) ; 
        big_cnt = atoi( argv[4] ) ; 
        is_divide = atoi( argv[5] ) ;
    } 
    printf( "method = %s , group_cnt = %d * (small:big = %d:%d), %s\n" ,
            method == 0 ? "DSA_batch" : "DSA_memcpy" , group_cnt , small_cnt , big_cnt , 
            is_divide == 0 ? "mixed" : "divided" ) ;

    vector<OffLen> test_set = genSeparate( small_cnt , big_cnt , group_cnt , is_divide ) ;
    test_dsa_batch( small_cnt , big_cnt , test_set , method ) ;
    return 0 ;
}