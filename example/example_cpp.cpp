#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "src/lightdsa.hpp"

int main(){
    #ifdef __cplusplus
        printf( "C++\n" ) ;
    #else
        printf( "C\n" ) ;
    #endif
    printf( "Hello World\n" ) ;
    // Create DSAbatch object
    DSAbatch dsa_batch ; // The default args are ( DEFAULT_BATCH_SIZE , DEFAULT_BATCH_CAPACITY )

    // Init arrays for memmove
    char* a = (char*)malloc( 0x1000000 ) ;
    char* b = (char*)malloc( 0x1000000 ) ;
    for( int i = 0 ; i < 0x1000000 ; i ++ ) a[i] = i ;

    // Use DSAbatch to submit multiple memmove operations
    for( int i = 0 ; i < 0x1000 ; i ++ )
        dsa_batch.submit_memmove( a + i * 0x1000 , b + i * 0x1000 , 0x1000 ) ;
    // The DSAbatch is asynchronous, so use wait to wait for all operations done
    dsa_batch.wait() ;

    // Check if correct
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        if( a[i] != b[i] ) {
            printf( "Error at %d\n" , i ) ;
            break ;
        }
    }
    printf( "Batch Done\n" ) ;

    // Create DSAop object
    DSAop dsa_memmove ; 
    // Init arrays for memmove
    memset( b , 0 , 0x1000000 ) ;

    // Use DSAop to submit single memmove operation
    for( int i = 0 ; i < 0x1000 ; i ++ )
        // Here we use the sync version memmove, so DSAop_wait is not needed
        dsa_memmove.sync_memmove( b + i * 0x1000 , a + i * 0x1000 , 0x1000 ) ;

    // Check if correct
    for( int i = 0 ; i < 0x1000000 ; i ++ ) {
        if( a[i] != b[i] ) {
            printf( "Error at %d\n" , i ) ;
            break ;
        }
    }
    printf( "Memmove Done\n" ) ;

}