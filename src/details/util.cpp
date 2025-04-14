#include <sys/time.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include "util.hpp"

// return double timeStamp (s)
double timeStamp(){
    timeval tv ;
    gettimeofday( &tv , NULL ) ;
    return tv.tv_sec + tv.tv_usec / 1000000.0 ;
}

// return uint64_t timeStamp (ns)
uint64_t timeStamp_hires(){ 
    //Cast the time point to ms, then get its duration, then get the duration's count.
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();  
}

// FNV1a hash for c string
#define c_offsetBasis 14695981039346656037ULL
#define c_FNVPrime 1099511628211ULL 
uint64_t FNV_hash( void* bs_, size_t len ) {
    register uint64_t h  = c_offsetBasis;
    register uint8_t *bs = (uint8_t*)bs_ ;
    for (register size_t i = 0; i < len; ++i) {
        h = h ^ bs[i];
        h = h * c_FNVPrime;
    }
    return h;
}
#undef c_offsetBasis
#undef c_FNVPrime

/* Fast random numbers : Galois LFSR 32bit random numbers */
// http://www.cse.yorku.ca/~oz/marsaglia-rng.html
Galois_LFSR::Galois_LFSR(){
    lfsr = 1 ;
}

void Galois_LFSR::set_LFSR_number( uint32_t lfsr_ ){
    lfsr = lfsr_ ;
} 

size_t extract_bit( size_t num , size_t bits ){
    size_t extract = 0 ;
    for( int i = 0 , evict_cnt = 0 ; i < 64 ; i ++ ){
        if( bits&(1ULL<<i) ){
            extract |= ( num&(1ULL<<i) ) ? ( 1ULL << evict_cnt ) : 0 ;
            evict_cnt ++ ;
        }
    } return extract ;
}

size_t expand_bit( size_t num , size_t bits ){
    size_t expand = 0 ;
    for( int i = 0 , evict_cnt = 0 ; i < 64 ; i ++ ){
        if( (bits&(1ULL<<i)) ){
            expand |= ( num & (1ULL<<evict_cnt) ) ? ( 1ULL << i ) : 0 ;
            evict_cnt ++ ;
        }
    } return expand ;
}

void printbits_vv( void* addr , int bitcnt ){  
    int finbit = 0 ;
    while( bitcnt ){
        for( int i = 0 ; i <= 64 && i <= bitcnt ; i ++ ){
            if( i % 8 == 1 ){
                int tmp = finbit + i - 1 , wid = 0 ;
                while( tmp ) tmp /= 10 , wid ++ ;
                for( int widtmp = wid ; widtmp > 1 ; widtmp -= 2 ) printf( "\b" ) ;
                printf( "%d" , finbit + i - 1 ) ; 
                for( int widtmp = wid - 1 ; widtmp > 1 ; widtmp -= 2 ) i ++ ;
            }
            printf( " " ) ;
        } puts( "" ) ;

        for( int i = 1 ; i < 64 && i < bitcnt ; i ++ ) printf( ( i % 8 == 1 ) ? "|v" : "-" ) ; 
        puts( "-" ) ;
        
        for( int i = 0 ; i < 64 && bitcnt ; i ++ , bitcnt -- ){ 
            int byte = ( finbit + i ) / 8 ; 
            if( i % 8 == 0 ) printf( "|" ) ;
            if( ( *((uint8_t*)addr+byte) ) & ( 1 << (i%8) ) ) printf( "1" ) ;
            else printf( "0" ) ;
        }
        puts( "|" ) ; 
        finbit += 64 ;
    } 
}

int invld_range(void *base, uint64_t len) {
	int rc; 
	rc = mprotect(base, len, PROT_READ);
	if (rc) {
		rc = errno;
		printf("mprotect1 error: %s\n", strerror(errno)) ; fflush( stdout ) ;
		return -rc;
	} 
	rc = mprotect(base, len, PROT_READ | PROT_WRITE);
	if (rc) {
		rc = errno;
		printf("mprotect2 error: %s\n", strerror(errno)) ; fflush( stdout ) ;
		return -rc;
	} 
	return 0;
}

void printf_RGB(int r, int g, int b, const char* format, ...) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\033[0m");
}

void printf_RGB( int rgbhex , const char* format , ... ){
    int r = ( rgbhex >> 16 ) & 0xff ;
    int g = ( rgbhex >> 8 ) & 0xff ;
    int b = ( rgbhex >> 0 ) & 0xff ;
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\033[0m");
}