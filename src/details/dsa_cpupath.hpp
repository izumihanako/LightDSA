#ifndef QWQ_DSA_DSA_CPUPATH
#define QWQ_DSA_DSA_CPUPATH

#include <cstdint>
#include <cstddef>
#include <linux/idxd.h>
#include "dsa_batch_redistribute.hpp"

void memmove_cpu( void *dest , const void* src , size_t len , bool flush = false ) ;
void memfill_cpu( void *dest , uint64_t pattern , size_t len  , bool flush = false ) ;
int compare_cpu( void *dest , void* src , size_t len ) ;
int compval_cpu( void *dest , uint64_t pattern , size_t len ) ;
void flush_cpu( void *dest , size_t len ) ;

void do_by_cpu( dsa_hw_desc *desc , dsa_completion_record *comp = nullptr ) ;
void do_by_cpu( const dsa_rdstrb_entry &entry ) ;

#endif