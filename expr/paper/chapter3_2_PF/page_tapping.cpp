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
__always_inline void tap_read ( char* addr ){ volatile int x = (*addr) ; (void)(x) ; }

vector<copy_meta> genCopies( char* src_arr , char* dest_arr , size_t array_len , size_t transfer_size ){
    vector<copy_meta> res ; 
    size_t offset = 0 ;
    while( true ){
        if( offset + transfer_size > array_len ) break ;
        copy_meta m = (copy_meta){ src_arr + offset , dest_arr + offset , transfer_size } ;
        res.push_back( m ) ;
        offset += transfer_size ;
    }
    // random_shuffle( res.begin() , res.end() ) ;
    return res ;
}

#pragma GCC diagnostic ignored "-Wunused-result"
uint64_t pattern_zero = 0 , pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_zero & 0xff ; 
void do_copy( int method , int op_type ){  
    DSAbatch batch ;
    double avg_time = 0 ;
    double avg_speed = 0 ;
    if( method == 0 ) {
        printf( "no tap\n" ) ; fflush( stdout ) ;
    } else if( method == 1 ) {
        printf( "tap all\n" ) ; fflush( stdout ) ;
    } else if( method == 2 ) {
        printf( "tap needed\n" ) ; fflush( stdout ) ;
    } else if( method == 3 ){
        printf( "tap only, no copy\n" ) ; fflush( stdout ) ;
    } else if( method == 4 ){
        printf( "tap needed with large page\n" ) ; fflush( stdout ) ;
    } else if( method == 5 ){
        printf( "tap only with large page\n" ) ; fflush( stdout ) ;
    } else {
        printf( "unknown method %d\n" , method ) ; fflush( stdout ) ;
        return ;
    }
 
    for( int i = 0 , warmup = 0 ; i < REPEAT ; i ++ ){
        if( method >= 4 ){ 
            system( "echo 4096 > /proc/sys/vm/nr_hugepages" ) ; 
            src_arr  = (char*) mmap( NULL , array_len , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;
            dest_arr = (char*) mmap( NULL , array_len , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;
            if( src_arr == MAP_FAILED || dest_arr == MAP_FAILED ) {
                printf( "mmap failed\n" ) ; fflush( stdout ) ;
                return ;
            }
        } else {
            src_arr =  (char*) aligned_alloc( 4096 , array_len ) ;
            dest_arr = (char*) aligned_alloc( 4096 , array_len ) ; 
        }
        if( op_type == 0 ){         // memmove
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i ; // dest will PF
        } else if( op_type == 1 ){  // memfill 
            // do not touch anything                                    // dest will PF
        }else if( op_type == 2 ){   // compare, do not set the dest, so it will PF
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ; // dest will PF
        } else if( op_type == 3 ){  // compval
            // do not touch anything                                    // src will PF;
        }
        vector<copy_meta> tasks = genCopies( src_arr , dest_arr , array_len , transfer_size ) ;


        uint64_t start = timeStamp_hires() ;
        if( method == 0 ) { 
            for( auto &m : tasks ) {
                if( op_type == 0 )      batch.submit_memmove( m.dest , m.src , m.len ) ;
                else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
                else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
                else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ; 
            }
            batch.wait() ;
        } else if( method == 1 ) { 
            for( size_t i = 0 ; i < array_len ; i += 4096 ) {
                if( op_type <= 1 )  tap_write( dest_arr + i ) ; 
                else if( op_type == 2 ) tap_read( dest_arr + i ) ;
                else                tap_read( src_arr + i ) ; 
            }
            for( auto &m : tasks ) {
                if( op_type == 0 )      batch.submit_memmove( m.dest , m.src , m.len ) ;
                else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
                else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
                else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ;
            }
            batch.wait() ;
        } else if( method == 2 ) { 
            for( auto &m : tasks ) {
                if( op_type <= 1 )  for( size_t i = 0 ; i < m.len ; i += 4096 ) tap_write( m.dest + i ) ; 
                else if( op_type == 2 ) for( size_t i = 0 ; i < m.len ; i += 4096 ) tap_read( m.dest + i ) ; 
                else                for( size_t i = 0 ; i < m.len ; i += 4096 ) tap_read( m.src + i ) ;
                if( op_type == 0 )      batch.submit_memmove( m.dest , m.src , m.len ) ;
                else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
                else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
                else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ;
            }
            batch.wait() ;
        } else if( method == 3 ){ // tap only
            for( auto &m : tasks ) { 
                if( op_type <= 1 )  for( size_t i = 0 ; i < m.len ; i += 4096 ) tap_write( m.dest + i ) ; 
                else if( op_type == 2 ) for( size_t i = 0 ; i < m.len ; i += 4096 ) tap_read( m.dest + i ) ; 
                else                for( size_t i = 0 ; i < m.len ; i += 4096 ) tap_read( m.src + i ) ;
            }
        } else if( method == 4 ){ 
            for( auto &m : tasks ) {
                if( op_type <= 1 )  for( size_t i = 0 ; i < m.len ; i += 2 * MB ) tap_write( m.dest + i ) ; 
                else if( op_type == 2 ) for( size_t i = 0 ; i < m.len ; i += 2 * MB ) tap_read( m.dest + i ) ; 
                else                for( size_t i = 0 ; i < m.len ; i += 2 * MB )     tap_read( m.src + i ) ;
                if( op_type == 0 )      batch.submit_memmove( m.dest , m.src , m.len ) ;
                else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
                else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
                else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ;
            }
            batch.wait() ;
        } else if( method == 5 ){ // tap only with large page
            for( auto &m : tasks ) {
                if( op_type <= 1 )  for( size_t i = 0 ; i < m.len ; i += 2 * MB )       tap_write( m.dest + i ) ; 
                else if( op_type == 2 ) for( size_t i = 0 ; i < m.len ; i += 2 * MB )   tap_read( m.dest + i ) ; 
                else                for( size_t i = 0 ; i < m.len ; i += 2 * MB )       tap_read( m.src + i ) ;
            }
        } else {
            printf( "unknown method %d\n" , method ) ; fflush( stdout ) ;
            return ;
        }
        if( warmup >= 2 ){
            uint64_t end = timeStamp_hires() ;
            double time_seconds = ( end - start ) / 1000000000.0 , speed = ( array_len / time_seconds ) / GB ;
            // printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ;
            avg_time += time_seconds / REPEAT ;
            avg_speed += speed / REPEAT ;
        } else {
            warmup ++ ;
            i -- ;
        }
        if( method >= 4 ){
            munmap( src_arr , array_len ) ;
            munmap( dest_arr , array_len ) ;
            system( "echo 0 > /proc/sys/vm/nr_hugepages" ) ; 
        } else {
            free( src_arr ) ;
            free( dest_arr ) ;
        } 
    }
    batch.print_stats() ;
    printf( "average time: %.3f seconds, average speed: %.3f GB/s\n" , avg_time , avg_speed ) ; fflush( stdout ) ;
    return ;
}

DSAop __ ;
int main(){  
    // for( int me = 0 ; me <= 5 ; me ++ ){
    //     for( int op = 0 ; op <= 3 ; op ++ ){ 
    for( int me = 0 ; me <= 0 ; me ++ ){
        for( int op = 1 ; op <= 1 ; op ++ ){ 
            method = me , op_type = op ;
            printf( "method = %s, op_type = %s, transfer_size = %s, REPEAT = %d \n" , 
                ( method == 0 ? "no tap" : ( method == 1 ? "tap all" : ( method == 2 ? "tap needed" : 
                ( method == 3 ? "tap only, no copy" : ( method == 4 ? "tap needed with large page" : "tap only with large page" ) ) ) ) ) ,
                ( op_type == 0 ? "memcpy" : ( op_type == 1 ? "memfill" : ( op_type == 2 ? "compare" : "compval" ) ) ) ,
                stdsiz(transfer_size).c_str() , REPEAT ) ; fflush( stdout ) ;
            do_copy( method , op_type ) ;
        }
    }
}