#ifndef QWQ_DSA_CONSTANT
#define QWQ_DSA_CONSTANT
#include <cstddef>
#include <cstdint> 
#include <linux/idxd.h>

constexpr size_t KB = 1024 ;
constexpr size_t MB = 1024 * KB ;
constexpr size_t GB = 1024 * MB ;

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