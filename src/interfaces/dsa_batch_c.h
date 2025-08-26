#ifndef QWQ_DSA_BATCH_C_H
#define QWQ_DSA_BATCH_C_H

#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
    extern "C" {
#endif

typedef struct DSAbatch DSAbatch;
DSAbatch* DSAbatch_create( int bsiz, int cap ) ;
void DSAbatch_destroy( DSAbatch* dsa ) ;
int DSAbatch_submit_comp_pattern( DSAbatch* dsa, void *src, uint64_t pattern, size_t len ) ;
int DSAbatch_submit_memfill( DSAbatch* dsa, void *dest, uint64_t pattern, size_t len ) ;
int DSAbatch_submit_memmove( DSAbatch* dsa, void *dest, const void* src, size_t len ) ;
int DSAbatch_submit_flush( DSAbatch* dsa, void *dest, size_t len ) ;
int DSAbatch_check( DSAbatch* dsa ) ;
void DSAbatch_wait( DSAbatch* dsa ) ;
void DSAbatch_printstats( DSAbatch* dsa ) ;
void DSAinit_agent( void ) ;

// only used when fork a process
void DSAinit_new_agent( void ) ;

#ifdef __cplusplus
    }
#endif

#endif