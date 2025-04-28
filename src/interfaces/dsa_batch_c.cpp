#include "dsa_batch.hpp"
#include "dsa_batch_c.h"
#include <cstdio>
 
extern "C" {

DSAbatch* DSAbatch_create( int bsiz, int cap ){
    return new DSAbatch( bsiz , cap ) ;
}

void DSAbatch_destroy( DSAbatch* dsa ){
    delete dsa ;
}

int DSAbatch_submit_comp_pattern( DSAbatch* dsa, void *src, uint64_t pattern, size_t len ){
    return dsa->submit_comp_pattern( src , pattern , len ) ;
}

int DSAbatch_submit_memfill( DSAbatch* dsa, void *dest, uint64_t pattern, size_t len ){
    return dsa->submit_memfill( dest , pattern , len ) ;
}

int DSAbatch_submit_memcpy( DSAbatch* dsa, void *dest, const void* src, size_t len ){
    return dsa->submit_memcpy( dest , src , len ) ;
}

int DSAbatch_submit_flush( DSAbatch* dsa, void *dest, size_t len ){
    return dsa->submit_flush( dest , len ) ;
}

int DSAbatch_check( DSAbatch* dsa ){
    return dsa->check() ;
}

void DSAbatch_wait( DSAbatch* dsa ){
    dsa->wait() ;
    return ;
}

void DSAbatch_printstats( DSAbatch* dsa ){
    dsa->print_stats() ;
    return ; 
}

void DSAinit_agent( void ){
    DSAagent::get_instance() ;
    return ;
}

void DSAinit_new_agent( void ){
    DSAagent::new_instance() ;
    return ;
}

}