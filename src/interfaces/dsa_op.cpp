#include "details/dsa_agent.hpp" 
#include "dsa_op.hpp"
#include "details/dsa_cpupath.hpp"
#include <cstdio>
#include <cstring>

DSAop::DSAop(){
    dtask.set_wq( DSAagent::get_instance().get_wq() ) ;
    to_dsa = to_cpu = 0 ;
    align_sum = 0 ;
} ;

bool DSAop::async_compare( const void *dest , const void* src , size_t len ) noexcept( true ){
    if( dtask.check() == false ) return false ; 
    to_dsa ++ ;
    dtask.set_op( DSA_OPCODE_COMPARE , const_cast<void*>(dest) , src , len ) ;
    dtask.do_op() ;
    return true ;
}

bool DSAop::async_comp_pattern( const void *src , uint64_t pattern , size_t len ) noexcept( true ){
    if( dtask.check() == false ) return false ; 
    to_dsa ++ ;
    dtask.set_op( DSA_OPCODE_COMPVAL , (void*)(uintptr_t)pattern , const_cast<void*>(src) , len ) ;
    dtask.do_op() ;
    return true ;
}

bool DSAop::async_memfill( void *dest , uint64_t pattern , size_t len ) noexcept ( true ){
    if( dtask.check() == false ) return false ; 
    #ifdef SHORT_TO_CPU
        if( len < 0x200 ) { 
            to_cpu ++ ;
            memfill_cpu( dest , pattern , len , _FLAG_CC_ ? 0 : IS_CPU_FLUSH ) ; 
            return true ;
        }
    #endif 
    uintptr_t dest_ = (uintptr_t) dest ;
    #ifdef DESCS_ADDRESS_ALIGNMENT
        size_t align64_diff = 0 ;
        if( dest_ & 0x3f ){  
            align64_diff = 0x40 - ( dest_ & 0x3f ) ;
            align64_diff = len < align64_diff ? len : align64_diff ;
            for( size_t i = 0 ; i + 8 <= align64_diff ; i += 8 )
                *(uint64_t*)( dest_ + i ) = pattern ; 
            if( align64_diff & 0x7 ){
                uint64_t tmp = ( align64_diff & 0x7 ) * 8 ;
                uint64_t new_pattern = ( pattern >> ( 64 - tmp ) ) | ( pattern << tmp ) ;
                *(uint64_t*)( dest_ + align64_diff - 8 ) = new_pattern ;
                pattern = new_pattern ;
            }
            len -= align64_diff ;
            dest_ += align64_diff ; 
        }
    #endif
    to_dsa ++ ; 
    dtask.set_op( DSA_OPCODE_MEMFILL , (void*)(dest_) , (void*)(uintptr_t)pattern , len ) ;
    dtask.do_op() ;
    #if defined(DESCS_ADDRESS_ALIGNMENT) and !defined(FLAG_CACHE_CONTROL) and defined(MUST_PERSIST_WRITE)
        _mm_clwb( dest ) ; // persistent in memory
    #endif
    return true ;
}

bool DSAop::async_memmove( void *dest , const void* src , size_t len ) noexcept( true ){ 
    if( dtask.check() == false ) return false ; 
    #if defined(SHORT_TO_CPU)
        if( len < 0x200 ) { 
            to_cpu ++ ;
            memmove_cpu( dest , src , len , _FLAG_CC_ ? 0 : IS_CPU_FLUSH ) ;
            return true ;
        }
    #endif 
    
    uintptr_t dest_ = (uintptr_t) dest , src_ = (uintptr_t)src ;
    #ifdef DESCS_ADDRESS_ALIGNMENT
        size_t align64_diff = 0 ;
        if( dest_ & 0x3f ){  
            align64_diff = 0x40 - ( dest_ & 0x3f ) ;
            align_sum += align64_diff ; 
            // db_task.add_op( DSA_OPCODE_MEMMOVE , (void*)(dest_) , (void*)(src_) , align64_diff ) ;
            memcpy( dest , src , align64_diff ) ;
            len -= align64_diff ;
            dest_ += align64_diff ;
            src_ += align64_diff ;  
        } 
    #endif
    to_dsa ++ ;  
    dtask.set_op( DSA_OPCODE_MEMMOVE , (void*)(dest_) , (void*)(src_) , len ) ;
    dtask.do_op() ;
    #if defined(DESCS_ADDRESS_ALIGNMENT) and !defined(FLAG_CACHE_CONTROL) and defined(MUST_PERSIST_WRITE)
        _mm_clwb( dest ) ; // persistent in memory
    #endif 
    return true ;
}

bool DSAop::async_flush( void *dest , size_t len ) noexcept( true ){
    if( dtask.check() == false ) return false ;
    uintptr_t dest_ = (uintptr_t) dest ; 
    #ifdef DESCS_ADDRESS_ALIGNMENT 
        // just flush a bigger range which aligned to cache line
        if( dest_ & 0x3f ){
            len += ( dest_ & 0x3f ) ;
            dest_ &= ( ~0x3f ) ; 
        }
        if( len & 0x3f ){
            len += 0x40 - ( len & 0x3f ) ;
        }
    #endif
    to_dsa ++ ;
    dtask.set_op( DSA_OPCODE_CFLUSH , (void*)(dest_) , len ) ;
    dtask.do_op() ;
    return true ;
}

void DSAop::sync_memcpy( void *dest , const void* src , size_t len ) noexcept( true ){ 
    async_memmove( dest , src , len ) ;
    wait() ;
}

void DSAop::sync_memfill( void *dest , uint64_t pattern , size_t len ) noexcept( true ){
    async_memfill( dest , pattern , len ) ;
    wait() ;
}

void DSAop::sync_flush( void *dest , size_t len ) noexcept( true ){
    async_flush( dest , len ) ;
    wait() ;
}

void DSAop::sync_comp_pattern( const void *src , uint64_t pattern , size_t len ) noexcept( true ){
    async_comp_pattern( src , pattern , len ) ;
    wait() ;
}

void DSAop::sync_compare( const void *dest , const void* src , size_t len ) noexcept( true ){
    async_compare( dest , src , len ) ;
    wait() ;
}

bool DSAop::check() {
    return dtask.check() ;
}

void DSAop::wait(){
    while( dtask.check() == false ) ;
} 

void DSAop::print_stats(){ 
    printf( "DSAop: to_cpu = %d, to_dsa = %d, align_sum = %lu\n", to_cpu , to_dsa , align_sum ) ;
    dtask.print_stats() ; 
}

// 0 is same, 1 is different
bool DSAop::compare_res(){
    return dtask.get_result() ;
}

size_t DSAop::compare_differ_offset(){
    return dtask.get_bytes_completed() ;
}