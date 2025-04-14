#ifndef QWQ_DSA_CONF_HPP
#define QWQ_DSA_CONF_HPP
  

constexpr size_t KB = 1024 ;
constexpr size_t MB = 1024 * KB ;
constexpr size_t GB = 1024 * MB ;

constexpr int DEFAULT_BATCH_CAPACITY = 10 ;
constexpr int DEFAULT_BATCH_SIZE = 64 ;

constexpr int DEFAULT_POOL_SIZE = 4 * MB ; 


// #define DESCS_ADDRESS_ALIGNMENT          /*** align desc write address to 64 bytes ***/

#define ALLOCATOR_CONTIGUOUS_ENABLE      /*** use Contiguous Allocation Strategy ***/
#define ALLOCATOR_USE_HUGEPAGE           /*** use HugePage for comps and descs allocation ***/


// #define FLAG_BLOCK_ON_FAULT              /*** use desc flag BLOCK_ON_FAULT ***/ 
// #define FLAG_CACHE_CONTROL               /*** use desc flag CACHE_CONTROL ***/
// #define FLAG_DEST_READBACK               /*** use desc flag DEST_READBACK ***/


#endif