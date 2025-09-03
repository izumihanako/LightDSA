#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/lightdsa_c.h"

int main(){
    #ifdef __cplusplus
        printf( "C++\n" ) ;
    #else
        printf( "C\n" ) ;
    #endif
    printf( "Hello World\n" ) ;

    // Create DSAbatch object
    DSAbatch *dsa_batch = DSAbatch_create( 32 , 20 ) ;

    // Init arrays for memmove
    char* a = (char*)malloc( 0x1000000 ) ;
    char* b = (char*)malloc( 0x1000000 ) ;
    for( int i = 0 ; i < 0x1000000 ; i ++ ) a[i] = i ;

    // Use DSAbatch to submit multiple memmove operations
    for( int i = 0 ; i < 0x1000 ; i ++ )
        DSAbatch_submit_memmove( dsa_batch , a + i * 0x1000 , b + i * 0x1000 , 0x1000 ) ;
    // The DSAbatch is asynchronous, so use wait to wait for all operations done
    DSAbatch_wait( dsa_batch ) ;

    // Check if correct
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        if( a[i] != b[i] ) {
            printf( "Error at %d\n" , i ) ;
            break ;
        }
    }
    printf( "Batch Done\n" ) ;

    // Create DSAop object
    DSAop *dsa_memmove = (DSAop*)DSAop_create() ; 
    // Init arrays for memmove
    memset( b , 0 , 0x1000000 ) ;

    // Use DSAop to submit single memmove operation
    for( int i = 0 ; i < 0x1000 ; i ++ )
        // Here we use the sync version memmove, so DSAop_wait is not needed
        DSAop_sync_memmove( dsa_memmove , b + i * 0x1000 , a + i * 0x1000 , 0x1000 ) ;

    // Check if correct
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        if( a[i] != b[i] ) {
            printf( "Error at %d\n" , i ) ;
            break ;
        }
    }
    printf( "Memmove Done\n" ) ;

}