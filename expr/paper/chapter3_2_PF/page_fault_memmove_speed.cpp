#include "src/async_dsa.hpp"

#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>  
#include <fcntl.h>
#include <string>
#include <cmath>
#include <sys/mman.h>
using namespace std ;

#pragma GCC diagnostic ignored "-Wunused-result"
int method = 0 , REPEAT = 10 , op_type ;
char* src_arr , *dest_arr ;
size_t array_len = 1 * GB , transfer_size = 32 * KB ;
struct copy_meta{
    char* src , *dest ;
    size_t len ;
} ; 

__always_inline void tap_write( char* addr ){ *addr = 0 ; }  
__always_inline void tap_read ( char* addr ){ volatile int x = (*addr) ; (void)(&x) ; }

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

vector<copy_meta> genCopies( char* src_arr , char* dest_arr , size_t array_len , size_t transfer_size ){
    vector<copy_meta> res ; 
    size_t offset = 0 ;
    while( true ){
        if( offset + transfer_size > array_len ) break ;
        copy_meta m = (copy_meta){ src_arr + offset , dest_arr + offset , transfer_size } ;
        res.push_back( m ) ;
        offset += transfer_size ;
    } 
    return res ;
}

uint64_t pattern_zero = 0 , pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_zero & 0xff ; 
DSAbatch batch ;
void memmove_with_major_page_fault(){ 
    system( "echo 0 > /proc/sys/vm/nr_hugepages" ) ; 
    printf( "major page fault\n" ) ; fflush( stdout ) ;
    dest_arr = (char*) aligned_alloc( 4096 , array_len ) ;
    const char* filename = "tmpfile.tmp" ;
    int fd = open( filename , O_CREAT | O_RDWR , 0666 ) ;
    if( fd < 0 ) {
        printf( "open %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    if( ftruncate( fd , array_len ) < 0 ) {
        printf( "ftruncate %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    src_arr = (char*) mmap( NULL , array_len , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 ) ; 
    
    if( op_type == 0 ){         // memmove
        for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = src_arr[i] = i ;
    } else if( op_type == 1 ){  // memfill 
        // do not touch anything
    }else if( op_type == 2 ){   // compare, do not set the dest, so it will PF
        for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = src_arr[i] = char_patt ;
    } else if( op_type == 3 ){  // compval
        for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = src_arr[i] = char_patt ;
    }

    msync( src_arr , array_len , MS_SYNC ) ;
    munmap( src_arr , array_len ) ;
    close( fd ) ;
    system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
    fd = open( filename , O_RDWR ) ;
    if( fd < 0 ) {
        printf( "open %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    src_arr = (char*) mmap( NULL , array_len , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0 ) ; 
    madvise( src_arr, array_len , MADV_RANDOM);
    if( src_arr == MAP_FAILED ) {
        printf( "mmap %s failed\n" , filename ) ; fflush( stdout ) ;
        return ;
    }
    system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
    // to simulate the situation that src are swapped out 
    // so src is the disk file
    vector<copy_meta> copy_metas = genCopies( src_arr , dest_arr , array_len , transfer_size ) ;
    uint64_t start = timeStamp_hires() ;
    for( auto& m : copy_metas ) { 
        if( op_type == 0 )      batch.submit_memcpy( m.dest , m.src , m.len ) ; 
        else if( op_type == 1 ) batch.submit_memfill( m.src , pattern_ , m.len ) ;
        else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
        else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ; 
    }
    batch.wait() ;
    uint64_t end = timeStamp_hires() ;
    double time_seconds = ( end - start ) / 1000000000.0 , speed = ( array_len / time_seconds ) / GB ;
    printf( "time: %.3f seconds, speed: %.3f GB/s, NO REPEAT\n" , time_seconds , speed ) ; fflush( stdout ) ;
    batch.print_stats() ;
    swap( src_arr , dest_arr ) ;
    munmap( dest_arr , array_len ) ;
    close( fd ) ;
    unlink( filename ) ;
}

void memmove_with_minor_page_fault(){
    printf( "minor page fault\n" ) ; fflush( stdout ) ; 
    double avg_time = 0 ;
    double avg_speed = 0 ;
    for( int run = 0 ; run < REPEAT ; run++ ) { 
        src_arr = (char*) aligned_alloc( 4096 , array_len ) ;
        dest_arr = (char*) aligned_alloc( 4096 , array_len ) ;
    
        if( op_type == 0 ){         // memmove
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i ; // dest will PF
        } else if( op_type == 1 ){  // memfill 
            // do not touch anything                                    // dest will PF
        }else if( op_type == 2 ){   // compare, do not set the dest, so it will PF
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ; // dest will PF
        } else if( op_type == 3 ){  // compval
            // do not touch anything                                    // src will PF;
        }
        
        system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
        vector<copy_meta> copy_metas = genCopies( src_arr , dest_arr , array_len , transfer_size ) ; 
        uint64_t start = timeStamp_hires() ;
        for( auto& m : copy_metas ) { 
            if( op_type == 0 )      batch.submit_memcpy( m.dest , m.src , m.len ) ;
            else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
            else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
            else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ; 
        }
        batch.wait() ;
        uint64_t end = timeStamp_hires() ; 
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( array_len / time_seconds ) / GB ;
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
    printf( "with ATS miss\n" ) ; fflush( stdout ) ;
    double avg_time = 0 ;
    double avg_speed = 0 ;
    for( int run = 0 ; run < REPEAT ; run ++ ){
        src_arr = (char*) aligned_alloc( 4096 , array_len ) ;
        dest_arr = (char*) aligned_alloc( 4096 , array_len ) ;
        (void)system( "echo 3 > /proc/sys/vm/drop_caches" ) ;

        if( op_type == 0 ){         // memmove
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = i ;
        } else if( op_type == 1 ){  // memfill 
            for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = 0 ;
        }else if( op_type == 2 ){   // compare
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = char_patt ;
        } else if( op_type == 3 ){  // compval
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ;
        }

        vector<copy_meta> copy_metas = genCopies( src_arr , dest_arr , array_len , transfer_size ) ;
        uint64_t start = timeStamp_hires() ; 
        for( auto& m : copy_metas ) { 
            if( op_type == 0 )      batch.submit_memcpy( m.dest , m.src , m.len ) ;
            else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
            else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
            else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ; 
        }
        batch.wait() ;
        uint64_t end = timeStamp_hires() ;
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( array_len / time_seconds ) / GB ;
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
    printf( "without page fault\n" ) ; fflush( stdout ) ;
    double avg_time = 0 ;
    double avg_speed = 0 ;
    system( "echo 4 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages" ) ;
    for( int run = 0 ; run < REPEAT ; run ++ ){
        char* dest_arr = (char*)mmap( NULL,  array_len,  PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT), 
            -1, 0 );
        src_arr = (char*)mmap( NULL,  array_len,  PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT), 
            -1, 0 );
        system( "echo 3 > /proc/sys/vm/drop_caches" ) ;
        if( op_type == 0 ){         // memmove
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = i ;
        } else if( op_type == 1 ){  // memfill 
            for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = 0 ;
        }else if( op_type == 2 ){   // compare
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = char_patt ;
        } else if( op_type == 3 ){  // compval
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ;
        }
        vector<copy_meta> copy_metas = genCopies( src_arr , dest_arr , array_len , transfer_size ) ;
        uint64_t start = timeStamp_hires() ;
        for( auto& m : copy_metas ) { 
            if( op_type == 0 )      batch.submit_memcpy( m.dest , m.src , m.len ) ;
            else if( op_type == 1 ) batch.submit_memfill( m.dest , pattern_ , m.len ) ;
            else if( op_type == 2 ) batch.submit_compare( m.dest , m.src , m.len ) ;
            else if( op_type == 3 ) batch.submit_comp_pattern( m.src , pattern_zero , m.len ) ; 
        }
        batch.wait() ;
        uint64_t end = timeStamp_hires() ;
        double time_seconds = ( end - start ) / 1000000000.0 , speed = ( array_len / time_seconds ) / GB ;
        printf( "time: %.3f seconds, speed: %.3f GB/s\n" , time_seconds , speed ) ; fflush( stdout ) ;
        avg_time += time_seconds / REPEAT ;
        avg_speed += speed / REPEAT ;
        munmap( dest_arr , array_len) ;
        munmap( src_arr , array_len) ;
    }
    batch.print_stats() ; fflush( stdout ) ;
    printf( "memmove without page fault\n" ) ; fflush( stdout ) ;
    printf( "average time: %.3f seconds, average speed: %.3f GB/s, REPEAT = %d\n" , avg_time , avg_speed , REPEAT ) ; fflush( stdout ) ;
}

int main(){ 
    system( "echo 0 > /proc/sys/vm/nr_hugepages" ) ;
    for( int me = 1 ; me <= 3 ; me ++ ){
        for( int op = 0 ; op <= 3 ; op ++ ){ 
            method = me , op_type = op ;
            printf( "method = %s, op_type = %s, transfer_size = %s, REPEAT = %d\n" , 
                ( method == 0 ? "major PF" : ( method == 1 ? "minor PF" : ( method == 2 ? "ATS miss" :  "no PF" ) ) ) ,
                ( op_type == 0 ? "memmove" : ( op_type == 1 ? "memfill" : ( op_type == 2 ? "compare" : "compval" ) ) ) ,
                stdsiz( transfer_size ).c_str() , REPEAT ) ;
            if( method == 0 ) memmove_with_major_page_fault() ;
            else if( method == 1 ) memmove_with_minor_page_fault() ;
            else if( method == 2 ) memmove_with_ats_miss() ;
            else if( method == 3 ) memmove_without_page_fault() ;
            system( "echo 0 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages" ) ;
            puts( "" ) ;
        }
    }
}
 