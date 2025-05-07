#include "src/async_dsa.hpp"

#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib> 
#include <sys/mman.h>
using namespace std ;

int method = 0 , op_type = 0 , REPEAT = 10 ;
char* src_arr , *dest_arr ;
size_t array_len = 1 * GB , transfer_size = 32 * KB ;


struct copy_meta{
    char* src , *dest ;
    size_t len ;
} ;

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

__always_inline void tap_write( char* addr ){ *addr = 0 ; }  
__always_inline void tap_read ( char* addr ){ volatile int x = (*addr) ; (void)(&x) ; }
  
void do_tap_huge(){  
    int x = system( "echo 4096 > /proc/sys/vm/nr_hugepages" ) ; 
    if( x ) puts( "error" ) ;
    src_arr  = (char*) mmap( NULL , array_len , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;
    dest_arr = (char*) mmap( NULL , array_len , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;

    uint64_t start = timeStamp_hires() ;
    for( size_t i = 0 ; i < array_len ; i += 2 * MB ) tap_read( src_arr + i ) ;
    for( size_t i = 0 ; i < array_len ; i += 2 * MB ) tap_read( dest_arr + i ) ;
    uint64_t end = timeStamp_hires() ;
    double time_seconds = ( end - start ) / 1000000000.0 , speed = ( array_len * 2 / time_seconds ) / GB ;
    printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ; 
    for( size_t i = 0 ; i < array_len ; i ++ ) if( src_arr[i] != 0 || dest_arr[i] != 0 ) {
        puts( "qwq" ) ;
    }
    munmap( src_arr , array_len ) ;
    munmap( dest_arr , array_len ) ;
    x = system( "echo 4096 > /proc/sys/vm/nr_hugepages" ) ; 
    if( x ) puts( "error" ) ;
    return ;
}

DSAop __ ;
int main(){  
    while( 1 ) do_tap_huge() ;
}