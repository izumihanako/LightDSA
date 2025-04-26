#ifndef QWQ_DSA_MEMCPY_C_H
#define QWQ_DSA_MEMCPY_C_H
#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct DSAmemcpy DSAmemcpy;
DSAmemcpy* DSAmemcpy_create() ;
void DSAmemcpy_destroy( DSAmemcpy* dsa ) ;
void* DSAmemcpy_do_async( DSAmemcpy* dsa, void *dest, const void* src, size_t len ) ;
int DSAmemcpy_check( DSAmemcpy* dsa ) ;
void DSAmemcpy_wait( DSAmemcpy* dsa ) ;
void* DSAmemcpy_do_sync( DSAmemcpy* dsa, void* dest, const void* src, size_t len ) ; 

#ifdef __cplusplus
    }
#endif

#endif
 