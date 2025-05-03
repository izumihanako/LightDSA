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


#define DESCS_QUEUE_RECYCLE_WINDOW_ENABLE /*** use recycle window ***/  
constexpr int QUEUE_RECYCLE_UNFINISHED_LIMIT = 25 ; /*** 每个WQ对应的batch回收探查区间长度 ***/

#define PAGE_FAULT_RESOLVE_TOUCH_ENABLE  /*** touch pages if frequent PF, touch leading pages before submit ***/
constexpr int DSA_RETRY_LIMIT = 2 ;         /*** 平均每次PF间隔小于128KB，并且次数大于RETRY_LIMIT，那就开始touch ***/
constexpr int DSA_PAGE_FAULT_FREQUENCY_LIMIT = 128 * KB ;

#define DESCS_INBATCH_REDISTRIBUTE_ENABLE   /*** redistribute descs in batch ***/

#define DESCS_ADDRESS_ALIGNMENT             /*** align desc write address to 64 bytes ***/

#define ALLOCATOR_CONTIGUOUS_ENABLE         /*** use Contiguous Allocation Strategy ***/
#define ALLOCATOR_USE_HUGEPAGE              /*** use HugePage for comps and descs allocation ***/
constexpr int DEFAULT_POOL_SIZE = 16 * MB ; /*** 每个WQ对应的内存池大小为POOL_SIZE ***/

// #define SHORT_TO_CPU                        /*** use CPU for short descs ***/
// #define OUTPUT_TO_FILE                      /*** will disable RGB output ***/




/****************** do not modify the following content ********************/

/********************************** fixs ***********************************/

// do not align when Cache Control is set
#if defined( DESCS_ADDRESS_ALIGNMENT ) && defined( FLAG_CACHE_CONTROL )
    #undef DESCS_ADDRESS_ALIGNMENT 
#endif

// do not redistribute when Readback is not set
#if defined( DESCS_INBATCH_REDISTRIBUTE_ENABLE ) && !defined( FLAG_DEST_READBACK )
    #undef DESCS_INBATCH_REDISTRIBUTE_ENABLE
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