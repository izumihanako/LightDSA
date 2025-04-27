#ifndef QWQ_DSA_CONF_HPP
#define QWQ_DSA_CONF_HPP
#include "dsa_constant.hpp"

/*** DSABatch 内的队列有 BATCH_CAPACITY 个Batch，每个Batch的大小为 BATCH_SIZE ***/
constexpr int DEFAULT_BATCH_CAPACITY = 20 ;
constexpr int DEFAULT_BATCH_SIZE = 32 ;

// #define DESCS_QUEUE_RECYCLE_WINDOW_ENABLE /*** use recycle window ***/

/*** 每个WQ对应的batch回收探查区间长度 ***/
constexpr int QUEUE_RECYCLE_UNFINISHED_LIMIT = 5 ;

/*** 如果平均每次PF间隔小于64KB，并且次数大于RETRY_LIMIT，那就直接fall back到CPU ***/
constexpr int DSA_RETRY_LIMIT = 4 ;
constexpr int DSA_PAGE_FAULT_FREQUENCY_LIMIT = 64 * KB ;

/*** 如果使用CONTIGUOUS分配策略，则每个WQ对应的内存池大小为POOL_SIZE ***/
constexpr int DEFAULT_POOL_SIZE = 4 * MB ; 

// #define DESCS_INBATCH_REDISTRIBUTE_ENABLE   /*** redistribute descs in batch ***/

// #define DESCS_ADDRESS_ALIGNMENT             /*** align desc write address to 64 bytes ***/

// #define ALLOCATOR_CONTIGUOUS_ENABLE         /*** use Contiguous Allocation Strategy ***/
#define ALLOCATOR_USE_HUGEPAGE              /*** use HugePage for comps and descs allocation ***/


#define FLAG_BLOCK_ON_FAULT                 /*** use desc flag BLOCK_ON_FAULT ***/ 
// #define FLAG_CACHE_CONTROL                  /*** use desc flag CACHE_CONTROL ***/
#define FLAG_DEST_READBACK                  /*** use desc flag DEST_READBACK ***/








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