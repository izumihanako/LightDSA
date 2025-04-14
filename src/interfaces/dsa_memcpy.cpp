#include "details/dsa_agent.hpp" 
#include "dsa_memcpy.hpp"
#include <cstdio>
#include <cstring>

DSAmemcpy::DSAmemcpy(){
    dtask.set_wq( DSAagent::get_instance().get_wq() ) ;        
} ;
    
void DSAmemcpy::do_async( void *dest , const void* src , size_t len ) noexcept( true ) {
    #ifdef BUILD_RELEASE
    if( len < 0x400 ) { 
        memcpy( dest , src , len ) ;
        return ;
    }
    #endif 
    uintptr_t dest_ = (uintptr_t) dest , src_ = (uintptr_t)src ;
    int align64_diff = 0 ;
    #ifdef DESCS_ADDRESS_ALIGNMENT
        if( dest_ & 0x3f ){ 
            int tmp = 0 ; 
            align64_diff = 0x40 - ( dest_ & 0x3f ) ; 
            align64_diff = len < align64_diff ? len : align64_diff ;
            memcpy( (void*)dest_ , (void*)src_ , align64_diff ) ;
            len -= align64_diff ;
            dest_ += align64_diff ;
            src_ += align64_diff ; 
        } 
    #endif
    dtask.set_op( DSA_OPCODE_MEMMOVE , (void*)(dest_ + align64_diff) , (void*)(src_ + align64_diff) , len ) ;
    dtask.do_op() ;
    #if defined(DESCS_ADDRESS_ALIGNMENT) and !defined(FLAG_CACHE_CONTROL) and defined(FLAG_DEST_READBACK)
        _mm_clflush( src ) ; // persistent in memory
    #endif 
    return ;
}

bool DSAmemcpy::check(){
    return dtask.check() ;
}

void DSAmemcpy::wait(){
    while( dtask.check() == false ){ 
    }
}

void* DSAmemcpy::do_sync( void* dest , const void* src , size_t len ) noexcept( true ) {
    do_async( dest , src , len ) ;
    wait() ;
    return dest ;
}

void* DSAmemcpy::operator() ( void* dest , const void* src , size_t len ) noexcept( true ) {
    return do_sync( dest , src , len ) ;
} 
