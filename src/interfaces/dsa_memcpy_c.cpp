#include "dsa_memcpy_c.h"
#include "dsa_memcpy.hpp"
extern "C" {
 
DSAmemcpy* DSAmemcpy_create(){
    return new DSAmemcpy() ;
}

void DSAmemcpy_destroy( DSAmemcpy* dsa ){
    delete dsa ;
}

void* DSAmemcpy_do_async( DSAmemcpy* dsa, void *dest, const void* src, size_t len ){
    dsa->do_async( dest , src , len ) ;
    return dest ;
}

int DSAmemcpy_check( DSAmemcpy* dsa ){
    return dsa->check() ;
}

void DSAmemcpy_wait( DSAmemcpy* dsa ) {
    dsa->wait() ;
    return ;
}

void* DSAmemcpy_do_sync( DSAmemcpy* dsa, void* dest, const void* src, size_t len ) {
    return dsa->do_sync( dest , src , len ) ;
}
 
} 