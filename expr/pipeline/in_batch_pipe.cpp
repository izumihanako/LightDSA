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
constexpr int REPEAT = 10 , bsiz = 32 ;
constexpr int BIG_LEN = 16 * KB , SMALL_LEN = 1024 ; 
int small_cnt = 1 , big_cnt = 1 , group_cnt = 100 , method = 0 , is_random = 0 , is_divide = 0 ;

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
 
vector<OffLen> genWorkload( int small_cnt , int big_cnt , int group_cnt , int is_random ) {
    vector<OffLen> small , big , res ;
    size_t offset = 0 ;
    int group_big_cnt = 0 , group_small_cnt = 0 ;
    for( int k = 0 ; k < bsiz ; ){
        for( int j = 0 ; k < bsiz && j < big_cnt ; j ++ , k ++ , group_big_cnt ++ ) ;
        for( int j = 0 ; k < bsiz && j < small_cnt ; j ++ , k ++ , group_small_cnt ++ ) ;
    }
    for( int i = 0 ; i < group_cnt ; i ++ ){ 
        for( int j = 0 ; j < group_small_cnt ; j ++ , offset += SMALL_LEN )
            small.push_back( OffLen( offset , offset , SMALL_LEN ) ) ; 
        for( int j = 0 ; j < group_big_cnt ; j ++ , offset += BIG_LEN )
            big.push_back( OffLen( offset , offset , BIG_LEN ) ) ;  
    }
    if( is_random ){
        random_shuffle( small.begin() , small.end() ) ;
        random_shuffle( big.begin() , big.end() ) ;
    }
    int small_id = 0 , big_id = 0 ;
    for( int i = 0 ; i < group_cnt ; i ++ ){
        if( is_divide == 0 ) { 
            for( int k = 0 ; k < bsiz ; ){
                for( int j = 0 ; k < bsiz && j < big_cnt   ; j ++ , k ++ ) res.push_back( big[big_id++] ) ;
                for( int j = 0 ; k < bsiz && j < small_cnt ; j ++ , k ++ ) res.push_back( small[small_id++] ) ;
            }
        } else if( is_divide == 1 ){
            for( int j = 0 ; j < group_big_cnt ; j ++ ) res.push_back( big[big_id++] ) ;
            for( int j = 0 ; j < group_small_cnt ; j ++ ) res.push_back( small[small_id++] ) ;
        } else if( is_divide == 2 ){
            for( int j = 0 ; j < group_small_cnt ; j ++ ) res.push_back( small[small_id++] ) ;
            for( int j = 0 ; j < group_big_cnt ; j ++ ) res.push_back( big[big_id++] ) ;
        } 
    }
    return res;
}

DSAmemcpy ___ ;
int main( int argc , char** argv ){ 
    if( argc < 7 ){ 
        printf("Usage: %s <method> <group_cnt> <small_cnt> <big_cnt> <random_addr> <is_divide>\n", argv[0]) ;
        printf( "method    : 0 is DSA_batch, 1 is DSA_memcpy\n" ) ;
        printf( "group_cnt : number of groups\n" ) ;
        printf( "small_cnt : small desc in a group\n" ) ;
        printf( "big_cnt   : big desc in a group\n" ) ;
        printf( "rand_addr : random in address, 0 no, 1 yes\n" ) ;
        printf( "is_divide : divide big/small desc. 0 mixed, 1 big first, 2 small first\n" ) ;
        return 0 ;
    } else {
        method = atoi( argv[1] ) ; 
        group_cnt = atoi( argv[2] ) ;
        small_cnt = atoi( argv[3] ) ; 
        big_cnt = atoi( argv[4] ) ; 
        is_random = atoi( argv[5] ) ;
        is_divide = atoi( argv[6] ) ;
    } 
    printf( "method = %s , group_cnt = %d * (small:big = %d:%d), %s, %s\n" ,
            method == 0 ? "DSA_batch" : "DSA_memcpy" , group_cnt , small_cnt , big_cnt , 
            is_random ? "random" : "seq" , is_divide == 0 ? "mixed" : (is_divide == 1 ? "big first" : "small first") ) ;
    vector<OffLen> test_set = genWorkload( small_cnt , big_cnt , group_cnt , is_random ) ;
    printf( "batch_siz = %d -> [ " , bsiz ) ;
    for( int i = 0 ; i < bsiz ; i ++ ){
        if( test_set[i].len == BIG_LEN ) printf_RGB( 0x00cc00 , "B" ) ;
        else printf_RGB( 0xcccc00 , "S" ) ;
    }
    printf( " ]\n" ) ; fflush( stdout ) ;
    test_dsa_batch( small_cnt , big_cnt , test_set , method ) ;
    return 0 ;
}