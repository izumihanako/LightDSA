#define FOR_C
#include <stdio.h>
#include "src/async_dsa.hpp"

int main(){
    #ifdef __cplusplus
        printf( "C++\n" ) ;
    #else
        printf( "C\n" ) ;
    #endif
    printf( "Hello World\n" ) ;


}