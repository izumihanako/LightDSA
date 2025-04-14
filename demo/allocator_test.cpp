#include <cstdio>
#include "src/details/dsa_agent.hpp"

int main(){
    DSAallocator allocator( 256 * KB ) ;
    for( int i = 0 ; i < 100 ; i ++ ){
        char ss[20] ;
        uintptr_t len ;
        scanf( "%s%lu" , ss , &len ) ;
        if( ss[0] == 'a' ){
            printf( "space @ %ld\n" , (uintptr_t) allocator.allocate( len ) ) ;
        } else if( ss[0] == 'f' ){
            allocator.deallocate( (void*)len ) ;
        }
        allocator.print_space() ; 
    }
}