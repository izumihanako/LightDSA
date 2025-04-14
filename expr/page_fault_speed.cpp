#include "src/async_dsa.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
using namespace std ;

double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;
double global_memcpy_time = 0 ;
 
constexpr int REPEAT = 1 ;
constexpr int bsiz = 64 , tdesc = 64 ; 
constexpr int ARRAY_LEN = 1073741824 ;

inline void touch_pf_rd( char* x , size_t len ){
    char *uplim = x + len , tmp ;
    for( ; x < uplim ; x += 4096 ) tmp = *x ;
    x = uplim - len ;
    if( len ) tmp = *(x+len-1) , *(x+len-1) = tmp ;
}

inline void touch_pf( char* x , size_t len ){ 
    if( len ) *(x+len-1) = 'A' ; 
    for(char *uplim = x + len ; x < uplim ; x += 4096 ) *x = 'A' ; 
}

void test_touch( int repeat , size_t len , bool warmup , bool wr ){
    double time_span = 0 ;
    for( int tmp = 0 ; tmp < 1 ; tmp ++ ){
        char* a = (char*)malloc( len ) ;
        double st_time = timeStamp_hires() ;
        if( wr ){
            touch_pf( a , len ) ;
            // touch_pf_rd( a , len ) ;
        } else {
            touch_pf_rd( a , len ) ;
            // touch_pf( a , len ) ;
        }
        double ed_time = timeStamp_hires() ;
        time_span += ( ed_time - st_time ) / repeat * ns_to_us ;
        free( a ) ;
    }
    if( warmup ) return ;

    double speed = 1.0 * len / MB / ( time_span * us_to_s ) ;
    printf( "           ; %s-touch cost %5.2lfus, speed %5.0fMB/s\n", 
            wr ? "Wr" : "Rd" , time_span , speed ) ; 
}

void test_memcpy( int repeat , size_t len , bool warmup ){
    double st_time , ed_time , touch_time , do_time ;
    double memcpy_time = 0 , memcpy_speed = 0 , memcpy_do = 0 ;
    double memcpy_touch = 0 , memcpy_touch_speed = 0 , memcpy_pure_speed = 0;

    char *a = (char*) malloc( ARRAY_LEN ) ;
    for( size_t i = 0 ; i < ARRAY_LEN ; i ++ ) a[i] = 'A' ;
    for( int tmp = 0 ; tmp < repeat ; tmp ++ ){ 
        int cnt = ARRAY_LEN / len ;
        char *b = (char*)malloc( ARRAY_LEN ) ;
        touch_time = do_time = 0 ;
        for( int i = 0 ; i < cnt ; i ++ ){
            st_time = timeStamp_hires() ;  
            touch_pf( b + i * len , len ) ;
            ed_time = timeStamp_hires() , touch_time += ed_time - st_time , st_time = ed_time ; 
            memcpy( b + i * len , a + i * len , len ) ; 
            ed_time = timeStamp_hires() , do_time += ed_time - st_time , st_time = ed_time ;
        }
        memcpy_touch += touch_time / cnt ;
        memcpy_do    += do_time / cnt ; 
        free( b ) ;
    }
    free( a ) ;
    memcpy_do *= ns_to_us / repeat , memcpy_touch *= ns_to_us / repeat ;
    memcpy_time = memcpy_do + memcpy_touch ;
    memcpy_speed = len / ( memcpy_time * us_to_s ) / MB ;
    memcpy_pure_speed = len / ( memcpy_do * us_to_s ) / MB ; 
    memcpy_touch_speed = len / ( memcpy_touch * us_to_s ) / MB ; 
    if( warmup ) return ; 
     
    printf( "           ; memcpy     cost %5.2lfus, speed %5.0fMB/s\n"
            "           ;            |- touc %7.2lfus -- pure speed %5.0fMB/s\n"
            "           ;            |- wait %7.2lfus -- pure speed %5.0fMB/s\n" 
            "\n",  
            memcpy_time , memcpy_speed ,
            memcpy_touch , memcpy_touch_speed ,
            memcpy_do , memcpy_pure_speed ) ;
    global_memcpy_time = memcpy_time ;
}

void test_dsa_single( int repeat , size_t len , bool warmup ){
    double st_time , ed_time , touch_time , do_time ;
    double DSA_do = 0 , DSA_touch = 0 ;
    double DSA_time = 0 , DSA_speed = 0 , DSA_pure_speed = 0 , DSA_touch_speed = 0 ;
    
    char *a = (char*) malloc( len ) ;
    for( size_t i = 0 ; i < len ; i ++ ) a[i] = 'A' ;
    DSAmemcpy single_xfer ;
    for( int tmp = 0 ; tmp < repeat ; tmp ++ ){
        int cnt = len / len ;
        char *b = (char*)malloc( len ) ;
        touch_time = do_time = 0 ;
        for( int i = 0 ; i < cnt ; i ++ ){
            st_time = timeStamp_hires() ; 
            touch_pf( b + i * len , len ) ;
            ed_time = timeStamp_hires() , touch_time += ed_time - st_time , st_time = ed_time ; 
            single_xfer.do_sync( b + i * len , a + i * len , len ) ;  
            ed_time = timeStamp_hires() , do_time += ed_time - st_time , st_time = ed_time ;  
        }
        DSA_do    += do_time / cnt ;  
        DSA_touch += touch_time / cnt ;
        free( b ) ; 
    }
    free( a ) ;
    DSA_do *= ns_to_us / repeat , DSA_touch *= ns_to_us / repeat ;
    DSA_time = DSA_do + DSA_touch ;
    DSA_speed = len / ( DSA_time * us_to_s ) / MB ; 
    DSA_pure_speed = len / ( DSA_do * us_to_s ) / MB ; 
    DSA_touch_speed = len / ( DSA_touch * us_to_s ) / MB ; 
    if( warmup ) return ;
 
    printf( "           ; DSA_single cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n" 
            "           ;            |- touc %7.2lfus -- pure speed %5.0fMB/s\n"
            "           ;            |- wait %7.2lfus -- pure speed %5.0fMB/s\n"
            "\n", 
            DSA_time , DSA_speed , global_memcpy_time - DSA_time ,  
            DSA_touch , DSA_touch_speed ,
            DSA_do , DSA_pure_speed ) ; 
}

void test_dsa_batch( int repeat , size_t len , int bsiz , int tdesc , bool warmup ){
    double st_time , ed_time , touch_time , do_time , submit_time ;
    double pure_batch_do = 0 , pure_batch_submit = 0 , pure_batch_touch = 0 ;
    double pure_batch_time = 0 , pure_batch_speed = 0 , pure_batch_touch_speed = 0 ;

    char *a = (char*) malloc( len * bsiz ) ;      
    a[len*bsiz-1] = 'A' ;
    for( size_t i = 0 ; i < len * bsiz ; i += 4096 ) a[i] = 'A' ;

    DSAbatch xfer( bsiz , DEFAULT_BATCH_CAPACITY ) ; 
    for( int tmp = 0 ; tmp < repeat ; tmp ++ ){ 
        xfer.db_task.clear() ;
        char *b = (char*)malloc( len * bsiz ) ;
        st_time = timeStamp_hires() ;
        for( int i = 0 ; i < tdesc ; i ++ ) touch_pf( b + ( i % bsiz ) * len , len ) ;
        ed_time = timeStamp_hires() , touch_time = ed_time - st_time , st_time = ed_time ; 
        for( int i = 0 ; i < tdesc ; i ++ ) xfer.submit_memcpy( b + ( i % bsiz ) * len , a + ( i % bsiz ) * len , len ) ; 
        ed_time = timeStamp_hires() , submit_time = ed_time - st_time , st_time = ed_time ; 
        xfer.db_task.wait() ;
        ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
        pure_batch_submit += ( submit_time ) / repeat / tdesc ;
        pure_batch_do += ( do_time ) / repeat / tdesc ;
        pure_batch_touch += touch_time / repeat / tdesc ;
        free( b ) ; 
    }
    free( a ) ;
    pure_batch_do *= ns_to_us , pure_batch_submit *= ns_to_us , pure_batch_touch *= ns_to_us ;
    pure_batch_time = pure_batch_do + pure_batch_submit + pure_batch_touch ;
    pure_batch_speed = len / ( pure_batch_time * us_to_s ) / MB ;
    pure_batch_touch_speed = len / ( pure_batch_touch * us_to_s ) / MB ; 
    if( warmup ) return ; 

    printf( "           ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n" 
            "           ;            |- touc %7.2lfus -- pure speed %5.0fMB/s\n"
            "           ;            |- subm %7.2lfus\n" 
            "           ;            |- wait %7.2lfus\n"
            "\n",
            pure_batch_time , pure_batch_speed , global_memcpy_time - pure_batch_time ,
            pure_batch_touch , pure_batch_touch_speed ,
            pure_batch_submit , 
            pure_batch_do ) ; 
}
 
string stdsiz( size_t siz ) {
    double regsiz = siz ;
    int regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    char rt[20] ;
    sprintf( rt , "%3.2f%2s" , regsiz , regstr[regcnt] ) ;
    return string( rt ) ;
}

int main(){   
    for( size_t len = 1024 , warmup = 0 ; len <= ( 1 << 30 ) ; len *= 2 ){
        if( !warmup ) printf( "Copy %s ;\n" , stdsiz( len ).c_str() ) ; 
        test_touch( REPEAT , len , warmup , 1 ) ;
        test_touch( REPEAT , len , warmup , 0 ) ;
        // test_memcpy( REPEAT , len , warmup ) ;
        // test_dsa_single( REPEAT , len , warmup ) ;
        // test_dsa_batch( REPEAT , len , bsiz , tdesc , warmup ) ;
        if( warmup ) { warmup -- ; len /= 2 ; continue ; }
    }
}

// g++ coroutine2_dsa.cpp -o coroutine2_dsa -lboost_coroutine -lboost_context -laccel-config -Wall -mcmodel=large