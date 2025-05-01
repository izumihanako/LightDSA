#include "src/async_dsa.hpp"

#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib> 
using namespace std ;

int method = 0 , REPEAT = 10 ;
char* src_arr , *dest_arr ;
size_t arr_size = 1 * GB , transfer_size = 256 * KB ;

struct copy_meta{
    char* src , *dest ;
    size_t len ;
} ;

__always_inline void tap( char* addr ){
    *addr = 0 ;
}

vector<copy_meta> genCopies( char* src_arr , char* dest_arr , size_t arr_size , size_t transfer_size ){
    vector<copy_meta> res ; 
    size_t offset = 0 ;
    while( offset < arr_size ){
        copy_meta m ;
        m.src = src_arr + offset ;
        m.dest = dest_arr + offset ;
        m.len = transfer_size ;
        if( offset + transfer_size >= arr_size ) {
            m.len = arr_size - offset ;
        }
        res.push_back( m ) ;
        offset += transfer_size ;
    }
    random_shuffle( res.begin() , res.end() ) ;
    return res ;
}

DSAbatch batch ;
void do_copy( int method ){  
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
    } else {
        printf( "unknown method %d\n" , method ) ; fflush( stdout ) ;
        return ;
    }
    for( int i = 0 ; i < REPEAT ; i ++ ){
        src_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
        dest_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
        for( size_t i = 0 ; i < arr_size ; i += 1 ) src_arr[i] = (char) (i % 256) ; 
        vector<copy_meta> tasks = genCopies( src_arr , dest_arr , arr_size , transfer_size ) ;
        uint64_t start = timeStamp_hires() ;
        if( method == 0 ) { 
            for( auto &m : tasks ) {
                batch.submit_memcpy( m.dest , m.src , m.len ) ;
            }
            batch.wait() ;
        } else if( method == 1 ) { 
            for( size_t i = 0 ; i < arr_size ; i += 4096 ) {
                tap( dest_arr + i ) ;
            }
            for( auto &m : tasks ) {
                batch.submit_memcpy( m.dest , m.src , m.len ) ;
            }
            batch.wait() ;
        } else if( method == 2 ) { 
            for( auto &m : tasks ) {
                for( size_t i = 0 ; i < m.len ; i += 4096 ) {
                    tap( m.dest + i ) ;
                }
                batch.submit_memcpy( m.dest , m.src , m.len ) ;
            }
            batch.wait() ;
        } else if( method == 3 ){ 
            for( auto &m : tasks ) {
                for( size_t i = 0 ; i < m.len ; i += 4096 ) {
                    tap( m.dest + i ) ;
                }
            } 
        } else {
            printf( "unknown method %d\n" , method ) ; fflush( stdout ) ;
            return ;
        }
        uint64_t end = timeStamp_hires() ;
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( arr_size / time_seconds ) / GB ;
        printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ;
        avg_time += time_seconds / REPEAT ;
        avg_speed += speed / REPEAT ;
        free( dest_arr ) ;
        free( src_arr ) ;
    }
    batch.print_stats() ;
    printf( "average time: %.3f seconds, average speed: %.3f GB/s, REPEAT = %d\n" , avg_time , avg_speed , REPEAT ) ; fflush( stdout ) ;
    return ;
}

int main( int argc , char* argv[] ){
    if( argc < 2 ) {
        printf( "usage: %s <method>\n" , argv[0] ) ; fflush( stdout ) ;
        printf( "method: 0 - no tap, 1 - tap all, 2 tap needed, 3 tap only no copy \n" ) ; fflush( stdout ) ;
        return 0 ;
    }
    method = atoi( argv[1] ) ;
    do_copy( method ) ; 
}