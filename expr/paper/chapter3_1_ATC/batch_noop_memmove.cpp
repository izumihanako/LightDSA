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
 
constexpr int REPEAT = 1 ;
int memmove_cnt = 0 , noop_cnt = 0 , memmove_pages = 1 ;
size_t ARRAY_LEN = 0 ;
int tdesc = 32768 , bsiz = 100000 ;

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

void test_dsa_batch(){
    ARRAY_LEN = memmove_pages * 4096 ;
    char *a_ = (char*)aligned_alloc( 4 * KB , ARRAY_LEN * 4 ) ;
    touch_pf( a_ , ARRAY_LEN * 4 ) ;
    char *a = a_ , *b = a + ARRAY_LEN * 2 ; 
     
    DSAbatch xfer( memmove_cnt + noop_cnt , 5 ) ;
    for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){   
        for( int i = 0 ; i < memmove_cnt ; i ++ ){
            xfer.submit_memmove( b , a , memmove_pages * 4096 ) ;
        }
        for( int i = 0 ; i < noop_cnt ; i ++ ){
            xfer.submit_noop();
        } 
        xfer.wait() ; 
    }
 
    printf( "Copy %s; %d desc * %s + %d no-op \n" , stdsiz( memmove_cnt * memmove_pages * 4096 ).c_str() , 
            memmove_cnt , stdsiz( memmove_pages ).c_str() , noop_cnt ) ;  
    free( a_ ) ;
}

DSAop ___ ;
int main( int argc , char *argv[] ){
    if( argc > 2 ){ 
        memmove_cnt = atoi( argv[1] ) ;
        noop_cnt = atoi( argv[2] ) ; 
    } else {
        printf( "Usage     : %s memmove_cnt noop_cnt\n" , argv[0] ) ;
        printf( "memmove_cnt : num of memmove operation\n" ) ;
        printf( "noop_cnt    : num of noop operation\n" ) ; 
        return 0 ;
    }

    printf( "DSA memmove cnt %d, noop cnt = %d\n" , memmove_cnt , noop_cnt ) ; fflush(stdout) ;
    printf( "Batch descriptor list span %d pages\n", ( memmove_cnt + noop_cnt + 63 ) / 64 ) ;
    test_dsa_batch() ;
}

//