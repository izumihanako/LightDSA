#ifndef QWQ_DSA_CONF_HPP
#define QWQ_DSA_CONF_HPP
#include "dsa_constant.hpp"

/********************************** conf ***********************************/

/*** DSABatch 内的队列有 BATCH_CAPACITY 个Batch，每个Batch的大小为 BATCH_SIZE ***/
constexpr int DEFAULT_BATCH_CAPACITY = 80 ;
constexpr int DEFAULT_BATCH_SIZE = 32 ;

// #define FLAG_BLOCK_ON_FAULT                 /*** use desc flag BLOCK_ON_FAULT ***/ 
// #define FLAG_CACHE_CONTROL                  /*** use desc flag CACHE_CONTROL ***/
#define FLAG_DEST_READBACK                  /*** use desc flag DEST_READBACK ***/

// #define DESCS_OUT_OF_ORDER_RECYCLE_ENABLE /*** use recycle window ***/  
constexpr int OUT_OF_ORDER_RECYCLE_T_INIT = 25 ; /*** T_init value ***/

// #define INTERLEAVED_PAGEFAULT_ENABLE     /*** touch pages if frequent PF, touch leading pages before submit ***/
constexpr int DSA_PF_LEN_LIMIT = ( 128 * KB ) ;
constexpr int DSA_PAGE_FAULT_TOUCH_LEN = ( 128 * KB ) ;

// #define DESCS_INBATCH_DESCRIPTORS_MIXING_ENABLE   /*** mix descs in batch ***/

// #define DESCS_ADDRESS_ALIGNMENT             /*** align desc write address to 64 bytes ***/

// #define ALLOCATOR_CONTIGUOUS_ENABLE         /*** use Contiguous Allocation Strategy ***/
#define ALLOCATOR_USE_HUGEPAGE              /*** use HugePage for comps and descs allocation ***/
constexpr int DEFAULT_POOL_SIZE = 16 * MB ; /*** memory pool size for each WQ is POOL_SIZE ***/

// #define SHORT_TO_CPU                        /*** use CPU for short descs ***/
// #define MUST_PERSIST_WRITE                  /*** ensure every CPU write is write back ***/
#define OUTPUT_TO_FILE                      /*** will disable RGB output ***/


/****************** do not modify the following content ********************/

/********************************** fixs ***********************************/

// do not align when Cache Control is set
#if defined( DESCS_ADDRESS_ALIGNMENT ) && defined( FLAG_CACHE_CONTROL )
    #undef DESCS_ADDRESS_ALIGNMENT 
#endif

// do not redistribute when Readback is not set
#if defined( DESCS_INBATCH_DESCRIPTORS_MIXING_ENABLE ) && !defined( FLAG_DEST_READBACK )
    #undef DESCS_INBATCH_DESCRIPTORS_MIXING_ENABLE
#endif

/********************************** defs ***********************************/

#ifdef FLAG_BLOCK_ON_FAULT
    constexpr uint32_t _FLAG_BOF_ = IDXD_OP_FLAG_BOF ;
#else
    constexpr uint32_t _FLAG_BOF_ = 0 ;
#endif

#ifdef FLAG_CACHE_CONTROL
    constexpr uint32_t _FLAG_CC_ = IDXD_OP_FLAG_CC ;
#else
    constexpr uint32_t _FLAG_CC_ = 0 ;
#endif

#ifdef FLAG_DEST_READBACK
    constexpr uint32_t _FLAG_DRDBK_ = IDXD_OP_FLAG_DRDBK ;
#else
    constexpr uint32_t _FLAG_DRDBK_ = 0 ;
#endif

#ifdef MUST_PERSIST_WRITE
    constexpr uint32_t IS_CPU_FLUSH = 1 ;
#else
    constexpr uint32_t IS_CPU_FLUSH = 0 ;
#endif

constexpr uint32_t _FLAG_CRAV_ = IDXD_OP_FLAG_CRAV ; // Request Completion Record 
constexpr uint32_t _FLAG_RCR_ = IDXD_OP_FLAG_RCR ;   // Completion Record Address Valid 

constexpr uint32_t DSA_NOOP_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ ;
constexpr uint32_t DSA_MEMMOVE_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ | _FLAG_BOF_ | _FLAG_CC_ | _FLAG_DRDBK_ ;
constexpr uint32_t DSA_MEMFILL_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ | _FLAG_BOF_ | _FLAG_CC_ | _FLAG_DRDBK_ ;
constexpr uint32_t DSA_COMPARE_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ | _FLAG_BOF_ ;
constexpr uint32_t DSA_COMPVAL_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ | _FLAG_BOF_ ;
constexpr uint32_t DSA_CFLUSH_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ | _FLAG_BOF_ ;
constexpr uint32_t DSA_TRANSL_FETCH_FLAG = _FLAG_CRAV_ | _FLAG_RCR_ | _FLAG_BOF_ | _FLAG_CC_ ;


#endif