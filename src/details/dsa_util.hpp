#ifndef QWQ_DSA_UTIL_HPP
#define QWQ_DSA_UTIL_HPP
 
#include <x86intrin.h> 
#include <linux/idxd.h>
#include <accel-config/libaccel_config.h>

struct delta_rec {
	uint16_t off;
	uint64_t val;
} __attribute__((__packed__));

inline int enqcmd( volatile void *reg , dsa_hw_desc *desc ){
    uint8_t retry ; 
    asm volatile( ".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                  "setz %0\t\n"
                  :"=r"(retry)
                  :"a" (reg), "d" (desc) ) ; 
    return (int) retry ;
}

inline void movdir64b(void *dst, const void *src){
	asm volatile( ".byte 0x66, 0x0f, 0x38, 0xf8, 0x02\t\n"
		          : 
                  :"a" (dst), "d" (src) ) ;
}

inline void submit_desc(void *wq_portal, int dedicated,
		struct dsa_hw_desc *hw) { 
	if (dedicated) movdir64b(wq_portal, hw);
	else /* retry infinitely, a retry param is not needed at this time */
		while (enqcmd(wq_portal, hw) ) {
            _mm_pause() ;
        }
}

__always_inline uint8_t op_status( volatile uint8_t &status ){
    return status & DSA_COMP_STATUS_MASK ;
}

const char* dsa_op_str( uint32_t opcode ) ;

const char* dsa_comp_status_str( uint32_t opcode ) ;

int open_wq( accfg_wq *wq ) ;

void* wq_mmap( accfg_wq *wq ) ;

int find_avail_wq_portal( void *& wq_portal , int wq_type ) ;

static __always_inline uintptr_t next_cache_line(uintptr_t curr_addr){
	uintptr_t off = curr_addr & 0xfff;
	uintptr_t base = curr_addr & ~0xfff; 
	off += 0x40;
	off &= 0xfff; 
	return base + off;
}

static __always_inline void* next_portal( void* portal ){
    return (void*)next_cache_line( (uintptr_t)portal ) ; 
}

void print_desc( dsa_hw_desc &desc ) ;

#endif 