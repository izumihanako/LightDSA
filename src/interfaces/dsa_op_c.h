#ifndef QWQ_DSA_OP_C_H
#define QWQ_DSA_OP_C_H
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct DSAop DSAop;
DSAop* DSAop_create() ;
void DSAop_destroy( DSAop* dsa ) ;
int  DSAop_async_memcpy( DSAop* dsa, void *dest, const void* src, size_t len ) ;
int  DSAop_async_memfill( DSAop* dsa, void *dest, uint64_t pattern, size_t len ) ;
int  DSAop_async_flush( DSAop* dsa, void *dest, size_t len ) ;
int  DSAop_async_comp_pattern( DSAop* dsa, const void *src, uint64_t pattern, size_t len ) ;
int  DSAop_async_compare( DSAop* dsa, const void *dest, const void* src, size_t len ) ;

void DSAop_sync_memcpy( DSAop* dsa, void *dest, const void* src, size_t len ) ;
void DSAop_sync_memfill( DSAop* dsa, void *dest, uint64_t pattern, size_t len ) ;
void DSAop_sync_flush( DSAop* dsa, void *dest, size_t len ) ;
void DSAop_sync_comp_pattern( DSAop* dsa, void *src, uint64_t pattern, size_t len ) ;
void DSAop_sync_comp( DSAop* dsa, void *dest, const void* src, size_t len ) ;

int  DSAop_check( DSAop* dsa ) ;
void DSAop_wait( DSAop* dsa ) ;
void DSAop_print_stats( DSAop* dsa ) ; 

#ifdef __cplusplus
}
#endif

#endif
 