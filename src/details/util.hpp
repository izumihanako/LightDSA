#ifndef QWQ_UTIL_H
#define QWQ_UTIL_H 

#ifndef NULL
#define NULL 0
#endif


#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

// return double timeStamp (s)
double timeStamp() ;

// return uint64_t timeStamp (ns)
uint64_t timeStamp_hires() ;

// FNV1a hash for c string
uint64_t FNV_hash( void* bs, size_t len ) ;

/* Fast random numbers : Galois LFSR 32bit random numbers */
// http://www.cse.yorku.ca/~oz/marsaglia-rng.html
#define Galois_LFSR_MASK 0xd0000001u
#define Galois_LFSR_rand ( lfsr = ( lfsr >> 1 ) ^ ( unsigned int ) \
            ( ( 0 - ( lfsr & 1u ) ) & Galois_LFSR_MASK ) )
struct Galois_LFSR{
    uint32_t lfsr ;
    Galois_LFSR() ;
    void set_LFSR_number( uint32_t ) ;
    inline uint32_t lfsr32() {
        return Galois_LFSR_rand ;
    }
    inline uint64_t lfsr64(){
        uint64_t hi = Galois_LFSR_rand ;
        uint64_t lo = Galois_LFSR_rand ;
        return ( hi << 32 ) | lo ;
    }
} ;
#undef Galois_LFSR_MASK
#undef Galois_LFSR_rand

#define unlikely(x)    __builtin_expect(!!(x), 0)

// extract bits of num where mask is 1, and compact these bits
// return the extracted bits
// e.g. num = 0b11001100, bits = 0b00001111, return 0b00001100
size_t extract_bit( size_t num , size_t bits ) ;

// expand bits of num to where mask is 1. Mask is 0, then fill with 0
// e.g. num = 0b00001101, bits = 0b11011000, return 0b11001000
size_t expand_bit( size_t num , size_t bits ) ;

// print bits at addr, bitcnt is the number of bits to print
void printbits_vv( void* addr , int bitcnt ) ;

// invalid all the TLB entries in the range [base, base+len)
int invld_range(void *base, uint64_t len) ;

// print c format string with RGB color
void printf_RGB(int r, int g, int b, const char* format, ...) ;

// print c format string with RGB color
void printf_RGB( int rgbhex , const char* format, ...) ;

// trigger needed page fault for [addr, addr + len)
void touch_trigger_pf( char* addr_ , size_t len , int wr ) ;

#endif