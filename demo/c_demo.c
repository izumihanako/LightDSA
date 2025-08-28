#define FOR_C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/async_dsa_c.h"

int main(){
    #ifdef __cplusplus
        printf( "C++\n" ) ;
    #else
        printf( "C\n" ) ;
    #endif
    printf( "Hello World\n" ) ;
    DSAbatch *dsa = DSAbatch_create( 32 , 20 ) ;
    char* a = (char*)malloc( 0x1000000 ) ;
    char* b = (char*)malloc( 0x1000000 ) ;
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        a[i] = i ;
    }
    for( int i = 0 ; i < 0x1000 ; i ++ ){
        DSAbatch_submit_memmove( dsa , a + i * 0x1000 , b + i * 0x1000 , 0x1000 ) ;
    }
    DSAbatch_wait( dsa ) ;
    // check if correct
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        if( a[i] != b[i] ) {
            printf( "Error at %d\n" , i ) ;
            break ;
        }
    }
    printf( "batch Done\n" ) ;

    DSAop *memcpy_ = (DSAop*)DSAop_create() ;
    memset( b , 0 , 0x1000000 ) ;
    for( int i = 0 ; i < 0x1000 ; i ++ ){
        DSAop_sync_memmove( memcpy_ , b + i * 0x1000 , a + i * 0x1000 , 0x1000 ) ;
    } 
    // check if correct
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        if( a[i] != b[i] ) {
            printf( "Error at %d\n" , i ) ;
            break ;
        }
    }
    printf( "memcpy Done\n" ) ;

}