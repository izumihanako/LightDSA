#ifndef QWQ_DSA_BATCH_REDISTRIBUTE_HPP
#define QWQ_DSA_BATCH_REDISTRIBUTE_HPP
#include "dsa_conf.hpp" 

struct dsa_rdstrb_entry{
    dsa_opcode opcode ;
    uint32_t flags ;
	union {
		uint32_t	xfer_size;
		uint32_t	desc_count;
		uint32_t	region_size;
	} ;
    union { 
		uint64_t	src_addr; 
		uint64_t	pattern; 
		uint64_t	pattern_lower; 
	};
	union {
		uint64_t	dst_addr; 
		uint64_t	src2_addr;
		uint64_t	comp_pattern;
        uint64_t    region_stride ;
	};
    dsa_rdstrb_entry( dsa_opcode ) ; 
    dsa_rdstrb_entry( dsa_opcode , void* src , uint64_t len , uint64_t stride = 0 ) ;
    dsa_rdstrb_entry( dsa_opcode , void* , const void* , uint64_t ) ;
};


class DSAtask_redistribute{
    dsa_rdstrb_entry *nega_entries , *posi_entries ;
    int8_t *posi_credits , *nega_credits ;
    int nega_cnt , posi_cnt , bsiz , counter , sum_credit ;
    double credit , credit_fix ;

public :
    DSAtask_redistribute() ;
    DSAtask_redistribute( int bsiz_ ) ;
    ~DSAtask_redistribute() ;

    __always_inline bool empty() { return nega_cnt + posi_cnt == 0 ; }
    __always_inline bool should_submit() { return nega_cnt + posi_cnt == bsiz ; }
    void init( int bsiz_ ) ; 
    void push_back( const dsa_rdstrb_entry &entry ) ;
    dsa_rdstrb_entry pop() ;
} ;

#endif