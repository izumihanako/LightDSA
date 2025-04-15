#include "src/async_dsa.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>

double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;

constexpr int DSA_CNT = 4 , DSAmemcpy_cnt = 64;
constexpr int REPEAT = 1 ;
constexpr int bsiz = 64 , tdesc = 4096 , MAX_LEN = 16 * MB ; 
constexpr int ARRAY_LEN = bsiz * MAX_LEN ;

void test_DSA( char* a , char* b , int len , bool flush ){
    double st_time , ed_time , do_time , flush_time ;  
    double DSA_do = 0 , DSA_flush = 0 ;
    double DSA_time = 0 , DSA_speed = 0 , DSA_flush_speed = 0 ;
    DSAmemcpy xfer[DSAmemcpy_cnt] ; 
    DSAbatch qfer[DSA_CNT] ;
    for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
        st_time = timeStamp_hires() ;  
        for( int i = 0 ; i < tdesc ; i ++ ) {
            int id = i % DSAmemcpy_cnt ;
            xfer[id].wait() ;
            xfer[id].do_async( b + ( i % bsiz ) * len , a + ( i % bsiz ) * len , len ) ; 
        }
        for( int i = 0 ; i < DSAmemcpy_cnt ; i ++ ) xfer[i].wait() ;
        ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ;
        if( flush ){ 
            for( int i = 0 ; i < tdesc ; i ++ )
                qfer[i%DSA_CNT].submit_flush( b + ( i % bsiz ) * len , len ) ; 
            for( int i = 0 ; i < DSA_CNT ; i ++ ) qfer[i].wait() ; 
        }
        ed_time = timeStamp_hires() , flush_time = ed_time - st_time , st_time = ed_time ;
        DSA_do += ( do_time ) / REPEAT / tdesc ;
        DSA_flush += ( flush_time ) / REPEAT / tdesc ;
        // usleep( 20000 ) ;
    }
    DSA_do *= ns_to_us , DSA_flush *= ns_to_us ;
    DSA_time = DSA_do + DSA_flush ;
    DSA_speed = len / ( DSA_time * us_to_s ) / MB ;  
    DSA_flush_speed = DSA_flush < 0.01 ? 0 : len / ( DSA_flush * us_to_s ) / MB ;  

    int regsiz = len , regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    printf( "Copy %d%2s * %d, REPEAT = %d times\n" , regsiz , regstr[regcnt] , tdesc , REPEAT ) ;
    printf( "    ; DSA_single cost %5.2lfus, speed %5.0fMB/s\n"
            "    ;    |- do    %7.2lfus\n" 
            "    ;    |- flush %7.2lfus -(%d DSA) speed %5.0fMB/s\n" 
            "\n",  
            DSA_time , DSA_speed ,
            DSA_do , 
            DSA_flush , !flush ? 0 : DSA_CNT , DSA_flush_speed ) ;
    fflush( stdout ) ; 
}
 
void test_DSA_batch( char* a , char* b , int len , bool flush ){
    double st_time , ed_time , submit_time , do_time , flush_time ;  
    double DSA_batch_do = 0 , DSA_batch_submit = 0 , DSA_batch_flush = 0 ;
    double DSA_batch_time = 0 , DSA_batch_speed = 0 , DSA_flush_speed = 0 ;
    DSAbatch qfer[DSA_CNT] ;
    for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){  
        st_time = timeStamp_hires() ; 
        for( int i = 0 ; i < tdesc ; i ++ ) 
                qfer[0].submit_memcpy( b + ( i % bsiz ) * len , a + ( i % bsiz ) * len , len ) ;  
        ed_time = timeStamp_hires() , submit_time = ed_time - st_time , st_time = ed_time ;
        for( int i = 0 ; i < 1 ; i ++ ) qfer[i].wait() ;  
        ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ;
        if( flush ){ 
            for( int i = 0 ; i < tdesc ; i ++ )
                qfer[i%DSA_CNT].submit_flush( b + ( i % bsiz ) * len , len ) ; 
            for( int i = 0 ; i < DSA_CNT ; i ++ ) qfer[i].wait() ; 
        }
        ed_time = timeStamp_hires() , flush_time = ed_time - st_time , st_time = ed_time ; 
        DSA_batch_submit += ( submit_time ) / REPEAT / tdesc ;
        DSA_batch_do += ( do_time ) / REPEAT / tdesc ;
        DSA_batch_flush += ( flush_time ) / REPEAT / tdesc ;
        // usleep( 20000 ) ;
    }
    DSA_batch_do *= ns_to_us , DSA_batch_submit *= ns_to_us , DSA_batch_flush *= ns_to_us ;
    DSA_batch_time = DSA_batch_do + DSA_batch_submit + DSA_batch_flush ;
    DSA_batch_speed = len / ( DSA_batch_time * us_to_s ) / MB ;  
    DSA_flush_speed = DSA_batch_flush < 0.01 ? 0 : len / ( DSA_batch_flush * us_to_s ) / MB ;  

    int regsiz = len , regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    printf( "Copy %d%2s * %d, REPEAT = %d times\n" , regsiz , regstr[regcnt] , tdesc , REPEAT ) ;
    printf( "    ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s\n"
            "    ;            |- subm  %7.2lfus\n" 
            "    ;            |- wait  %7.2lfus\n" 
            "    ;            |- flush %7.2lfus -(%d DSA) speed %5.0fMB/s\n" 
            "\n",  
            DSA_batch_time , DSA_batch_speed ,
            DSA_batch_submit ,
            DSA_batch_do , 
            DSA_batch_flush , !flush ? 0 : DSA_CNT , DSA_flush_speed ) ;
    fflush( stdout ) ; 
}


int main(){
    char *a = (char*) aligned_alloc ( 64 , ARRAY_LEN ) ;
    char *b = (char*) aligned_alloc ( 2 , ARRAY_LEN ) ; 
    if( ((uintptr_t) b & 0x3f ) == 0 ) b += 0x20 ;
    for( int i = 0 ; i < ARRAY_LEN ; i ++ ) a[i] = i ; 
    for( int i = 0 ; i < ARRAY_LEN ; i += 1 ) b[i] = 2 ; 
    
    // while( 1 ) 
    for( size_t len = 2 * KB ; len <= 4 * KB ; len *= 2 ){
        // puts( "testing DSA with flush") ;
        // test_DSA( a , b , len , true ) ;
        // puts( "testing DSA batch with flush") ;
        // test_DSA_batch( a , b , len , true ) ;
        // puts( "testing DSA without flush") ;
        // test_DSA( a , b , 1 * KB , false ) ; 
        puts( "testing DSA batch without flush") ;
        test_DSA_batch( a , b , len , false ) ;  
    }

    free( a ) ;
    free( b ) ; 
}
