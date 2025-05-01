#include "src/async_dsa.hpp"

#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>  
#include <fcntl.h>
#include <sys/mman.h>
using namespace std ;

#pragma GCC diagnostic ignored "-Wunused-result"
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

vector<copy_meta> genCopy( char* src , char* dest , size_t len , size_t transfer_size ){
    vector<copy_meta> copy_metas ;
    size_t offset = 0 ;
    while( offset < len ) {
        copy_meta meta ;
        meta.src = src + offset ;
        meta.dest = dest + offset ;
        meta.len = ( len - offset ) > transfer_size ? transfer_size : ( len - offset ) ;
        copy_metas.push_back( meta ) ;
        offset += transfer_size ;
    }
    random_shuffle( copy_metas.begin() , copy_metas.end() ) ;
    return copy_metas ;
}

DSAbatch batch ;
void memmove_with_major_page_fault(){ 
    system( "echo 0 > /proc/sys/vm/nr_hugepages" ) ; 
    printf( "memmove with major page fault\n" ) ; fflush( stdout ) ;
    src_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
    for( size_t i = 0 ; i < arr_size ; i += 1 ) src_arr[i] = (char) (i % 256) ;
    const char* filename = "tmpfile.tmp" ;
    int fd = open( filename , O_CREAT | O_RDWR , 0666 ) ;
    if( fd < 0 ) {
        printf( "open %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    if( ftruncate( fd , arr_size ) < 0 ) {
        printf( "ftruncate %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    dest_arr = (char*) mmap( NULL , arr_size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 ) ; 
    for( size_t i = 0 ; i < arr_size ; i += 4096 ) dest_arr[i] = i ;
    msync( dest_arr , arr_size , MS_SYNC ) ;
    munmap( dest_arr , arr_size ) ;
    close( fd ) ;
    system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
    fd = open( filename , O_RDWR ) ;
    if( fd < 0 ) {
        printf( "open %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    dest_arr = (char*) mmap( NULL , arr_size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 ) ; 
    madvise( dest_arr, arr_size , MADV_RANDOM);
    if( dest_arr == MAP_FAILED ) {
        printf( "mmap %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    // to simulate the situation that src are swapped out
    swap( src_arr , dest_arr ) ;
    vector<copy_meta> copy_metas = genCopy( src_arr , dest_arr , arr_size , transfer_size ) ;

    uint64_t start = timeStamp_hires() ;
    for( auto& meta : copy_metas ) {
        batch.submit_memcpy( meta.dest , meta.src , meta.len ) ;
    }
    batch.wait() ;
    uint64_t end = timeStamp_hires() ; 
    double time_seconds = ( end - start ) / 1000000000.0 , speed = ( arr_size / time_seconds ) / GB ;
    printf( "time: %.3f seconds, speed: %.3f GB/s, NO REPEAT\n" , time_seconds , speed ) ; fflush( stdout ) ;
    batch.print_stats() ;
    swap( src_arr , dest_arr ) ;
    munmap( dest_arr , arr_size ) ;
    close( fd ) ;
    unlink( filename ) ;
}

void memmove_with_minor_page_fault(){
    printf( "memmove with minor page fault\n" ) ; fflush( stdout ) ; 
    double avg_time = 0 ;
    double avg_speed = 0 ;
    for( int run = 0 ; run < REPEAT ; run++ ) { 
        src_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
        dest_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
        for( size_t i = 0 ; i < arr_size ; i += 1 ) src_arr[i] = (char) (i % 256) ;
        system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
        vector<copy_meta> copy_metas = genCopy( src_arr , dest_arr , arr_size , transfer_size ) ; 
        uint64_t start = timeStamp_hires() ;
        for( auto& meta : copy_metas ) {
            batch.submit_memcpy( meta.dest , meta.src , meta.len ) ;
        }
        batch.wait() ;
        uint64_t end = timeStamp_hires() ; 
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( arr_size / time_seconds ) / GB ;
        printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ;
        avg_time += time_seconds / REPEAT ;
        avg_speed += speed / REPEAT ;
        free( dest_arr ) ;
        free( src_arr ) ;
    }
    batch.print_stats() ;
    printf( "memmove with minor page fault\n" ) ; fflush( stdout ) ; 
    printf( "average time: %.3f seconds, average speed: %.3f GB/s, REPEAT = %d\n" , avg_time , avg_speed , REPEAT ) ; fflush( stdout ) ;
}

void memmove_with_ats_miss(){ 
    printf( "memmove with ATS miss\n" ) ; fflush( stdout ) ;
    double avg_time = 0 ;
    double avg_speed = 0 ;
    for( int run = 0 ; run < REPEAT ; run ++ ){
        src_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
        dest_arr = (char*) aligned_alloc( 4096 , arr_size ) ;
        (void)system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
        for( size_t i = 0 ; i < arr_size ; i += 4096 ) tap( dest_arr + i ) ;
        for( size_t i = 0 ; i < arr_size ; i += 1 ) src_arr[i] = (char) (i % 256) ; 
        vector<copy_meta> copy_metas = genCopy( src_arr , dest_arr , arr_size , transfer_size ) ;
        uint64_t start = timeStamp_hires() ;
        for( auto& meta : copy_metas ) {
            batch.submit_memcpy( meta.dest , meta.src , meta.len ) ;
        }
        batch.wait() ;
        uint64_t end = timeStamp_hires() ;
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( arr_size / time_seconds ) / GB ;
        printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ;
        avg_time += time_seconds / REPEAT ;
        avg_speed += speed / REPEAT ;
        free( dest_arr ) ;
        free( src_arr ) ;
    }
    batch.print_stats() ;
    printf( "memmove with ATS miss\n" ) ; fflush( stdout ) ;
    printf( "average time: %.3f seconds, average speed: %.3f GB/s, REPEAT = %d\n" , avg_time , avg_speed , REPEAT ) ; fflush( stdout ) ;
}

void memmove_without_page_fault(){ 
    printf( "memmove without page fault\n" ) ; fflush( stdout ) ;
    double avg_time = 0 ;
    double avg_speed = 0 ;
    system( "echo 4 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages" ) ;
    for( int run = 0 ; run < REPEAT ; run ++ ){
        char* dest_arr = (char*) (char*) (char*)mmap( NULL,  arr_size,  PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT), 
            -1, 0 );
        src_arr = (char*) (char*)mmap( NULL,  arr_size,  PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT), 
            -1, 0 );
        system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
        for( size_t i = 0 ; i < arr_size ; i += 4096 ) tap( dest_arr + i ) ;
        for( size_t i = 0 ; i < arr_size ; i += 1 ) src_arr[i] = (char) (i % 256) ;
        vector<copy_meta> copy_metas = genCopy( src_arr , dest_arr , arr_size , transfer_size ) ;
        uint64_t start = timeStamp_hires() ;
        for( auto& meta : copy_metas ) {
            batch.submit_memcpy( meta.dest , meta.src , meta.len ) ;
        }
        batch.wait() ;
        uint64_t end = timeStamp_hires() ;
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( arr_size / time_seconds ) / GB ;
        printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ;
        avg_time += time_seconds / REPEAT ;
        avg_speed += speed / REPEAT ;
        munmap( dest_arr , arr_size) ;
        munmap( src_arr , arr_size) ;
    }
    batch.print_stats() ; fflush( stdout ) ;
    printf( "memmove without page fault\n" ) ; fflush( stdout ) ;
    printf( "average time: %.3f seconds, average speed: %.3f GB/s, REPEAT = %d\n" , avg_time , avg_speed , REPEAT ) ; fflush( stdout ) ;
}

int main( int argc , char* argv[] ){
    if( argc < 2 ) {
        printf( "usage: %s <method>\n" , argv[0] ) ; fflush( stdout ) ;
        printf( "method: 0 - major PF, 1 - minor PF, 2 ATS miss, 3 no miss\n" ) ; fflush( stdout ) ;
        return 0 ;
    }
    method = atoi( argv[1] ) ;
    system( "echo 0 > /proc/sys/vm/nr_hugepages" ) ;
    if( method == 0 ) memmove_with_major_page_fault() ;
    else if( method == 1 ) memmove_with_minor_page_fault() ;
    else if( method == 2 ) memmove_with_ats_miss() ;
    else if( method == 3 ) memmove_without_page_fault() ;
    system( "echo 0 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages" ) ;
}
 