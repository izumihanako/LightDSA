#ifndef QWQ_DSA_CONF_HPP
#define QWQ_DSA_CONF_HPP
#include "dsa_constant.hpp"
  
/*** DSABatch 内的队列有 BATCH_CAPACITY 个Batch，每个Batch的大小为 BATCH_SIZE ***/
constexpr int DEFAULT_BATCH_CAPACITY = 10 ;
constexpr int DEFAULT_BATCH_SIZE = 64 ;

/*** 如果使用CONTIGUOUS分配策略，则每个WQ对应的内存池大小为POOL_SIZE ***/
constexpr int DEFAULT_POOL_SIZE = 4 * MB ; 

// #define DESCS_ADDRESS_ALIGNMENT          /*** align desc write address to 64 bytes ***/

#define ALLOCATOR_CONTIGUOUS_ENABLE         /*** use Contiguous Allocation Strategy ***/
#define ALLOCATOR_USE_HUGEPAGE              /*** use HugePage for comps and descs allocation ***/


// #define FLAG_BLOCK_ON_FAULT              /*** use desc flag BLOCK_ON_FAULT ***/ 
// #define FLAG_CACHE_CONTROL               /*** use desc flag CACHE_CONTROL ***/
// #define FLAG_DEST_READBACK               /*** use desc flag DEST_READBACK ***/


#endif