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
 
constexpr int REPEAT = 1 ;
int COPY_LEN = 512 , is_shuffle = 0 , tdesc = 32768 , bsiz = 100000 ;
size_t ARRAY_LEN = 256 * MB ; //

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

void test_dsa_batch( vector<OffLen> test_set , char *a , char* b , int is_shuffle ){ 
    double st_time , ed_time , do_time ;
    double dsa_time = 0 , dsa_speed = 0 ;

    if( is_shuffle ) random_shuffle( test_set.begin() , test_set.end() ) ;

    int cnt = test_set.size() ;
    size_t tot_siz = 0 ;
    for( int i = 0 ; i < cnt ; i ++ ) tot_siz += test_set[i].len ;
    touch_pf( a , min( ARRAY_LEN * 2 , tot_siz ) ) ; touch_pf( b , min( ARRAY_LEN * 2 , tot_siz ) ) ; 

    char *mem = (char*) aligned_alloc( 4096 , bsiz * 4096 ) ; 
    DSAop** memcpys = new DSAop*[bsiz]; 
    for( int i = 0 ; i < bsiz ; i ++ )
        memcpys[i] = new (mem + i * 4096) DSAop() ;
    for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){  
        st_time = timeStamp_hires() ;  
        for( int i = 0 ; i < cnt ; i ++ ){
            int id = i % bsiz ;
            memcpys[id]->wait() ;  
            memcpys[id]->async_memmove( b + test_set[i].off_dest , a + test_set[i].off_src , test_set[i].len ) ;
        }
        for( int i = 0 ; i < bsiz ; i ++ ) memcpys[i]->wait() ; 
        ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
        dsa_time += ( do_time ) / REPEAT ;
    } 
    delete[] memcpys ;

    dsa_time *= ns_to_us ; 
    dsa_speed = tot_siz / ( dsa_time * us_to_s ) / MB ; 
    printf( "Copy %s; %d desc * %s \n" , stdsiz( tot_siz ).c_str() , cnt , stdsiz( COPY_LEN ).c_str() ) ;
    printf( "    ; DSA_single cost %5.2lfus, speed %5.0fMB/s\n" , dsa_time , dsa_speed ) ;
    puts( "" ) ;
} 
 
vector<OffLen> genCopySet( int tdesc ) {
    vector<OffLen> rt ;
    size_t offset = 0 ;
    for( int i = 0 ; i < tdesc ; i ++ ){
        offset = ( i & 1 ) ? COPY_LEN : 0 ;
        rt.emplace_back( 0 , offset , COPY_LEN ) ; 
    }
    return rt ;
}

DSAop ___ ;
int main( int argc , char *argv[] ){ 
    if( argc > 4 ){
        is_shuffle = atoi( argv[1] ) ;
        ARRAY_LEN = atoll( argv[2] ) * KB ;
        COPY_LEN = atoll( argv[3] ) ;
        tdesc = atoll( argv[4] ) ;
        if( argc > 5 ) bsiz = atoi(argv[5]) ;
    } else {
        printf( "Usage     : %s shuffle array_len copy_len tdesc\n" , argv[0] ) ;
        printf( "shuffle   : 0 no shuffle, 1 shuffle\n" ) ;
        printf( "array_len : KB\n" ) ;
        printf( "copy_len  : bytes\n" ) ;
        printf( "tdesc     : num of memcpy\n" ) ;
        printf( "bsiz(opt) : num of descs\n" ) ;
        return 0 ;
    }

    printf( "%s, array %s, copy %s * %d descs , bsiz = %d \n" , is_shuffle == 0 ? "no shuffle" : "shuffle" , 
        stdsiz( ARRAY_LEN ).c_str() , stdsiz( COPY_LEN ).c_str() , tdesc , bsiz ) ; fflush(stdout) ;

    char *a_ = (char*)aligned_alloc( 4 * KB , ARRAY_LEN * 4 ) ;
    touch_pf( a_ , ARRAY_LEN * 4 ) ;
    char *a = a_ , *b = a + ARRAY_LEN * 2 ; 
    
    vector<OffLen> test_set = genCopySet( tdesc ) ; 
    test_dsa_batch( test_set , a , b , is_shuffle ) ; 

    free( a_ ) ; 
}

// python3 perf_PIPE.py "./expr/cache/diff_desc_s_src_d_dst 1 8192 4096 1000 2"
// "Translation hits" 的数量约等于 tdesc 的数量，如果 copy_len 大于一个 page 那么 hit 总是 0
// 在这个例子中，src地址每三次Translation被重复访问一次才有hit，而每5次访问一次则无hit
// 这意味着DSA可能只有3或4个cache entry
