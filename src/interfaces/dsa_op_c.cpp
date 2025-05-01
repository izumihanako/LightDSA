#include "dsa_op_c.h"
#include "dsa_op.hpp" 
extern "C" {
 
DSAop* DSAop_create() {
    return new DSAop() ;
}

void DSAop_destroy( DSAop* dsa ) {
    delete dsa ;
}

int DSAop_async_memcpy( DSAop* dsa, void *dest, const void* src, size_t len ) {
    return dsa->async_memcpy( dest , src , len ) ;
}

int DSAop_async_memfill( DSAop* dsa, void *dest, uint64_t pattern, size_t len ) {
    return dsa->async_memfill( dest , pattern , len ) ;
}

int DSAop_async_flush( DSAop* dsa, void *dest, size_t len ) {
    return dsa->async_flush( dest , len ) ;
}

int DSAop_async_comp_pattern( DSAop* dsa, const void *src, uint64_t pattern, size_t len ) {
    return dsa->async_comp_pattern( src , pattern , len ) ;
}

int DSAop_async_compare( DSAop* dsa, const void *dest, const void* src, size_t len ) {
    return dsa->async_compare( dest , src , len ) ;
}

void DSAop_sync_memcpy( DSAop* dsa, void *dest, const void* src, size_t len ) {
    dsa->sync_memcpy( dest , src , len ) ;
}

void DSAop_sync_memfill( DSAop* dsa, void *dest, uint64_t pattern, size_t len ) {
    dsa->sync_memfill( dest , pattern , len ) ;
}

void DSAop_sync_flush( DSAop* dsa, void *dest, size_t len ) {
    dsa->sync_flush( dest , len ) ;
}

void DSAop_sync_comp_pattern( DSAop* dsa, void *src, uint64_t pattern, size_t len ) {
    dsa->sync_comp_pattern( src , pattern , len ) ;
}


void DSAop_sync_compare( DSAop* dsa, const void *dest, const void* src, size_t len ) {
    dsa->sync_compare( dest , src , len ) ;
}

} 