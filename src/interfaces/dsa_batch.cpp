#include "dsa_batch.hpp"
#include "details/dsa_agent.hpp" 
#include "details/dsa_cpupath.hpp"
#include <cstring>
 
DSAbatch::DSAbatch( int bsiz , int cap ):db_task( bsiz , cap ) {
    db_task.set_wq( DSAagent::get_instance().get_wq() ) ;
    to_dsa = to_cpu = 0 ; 
}

bool DSAbatch::submit_comp_pattern( void *src , uint64_t pattern , size_t len ) noexcept( true ){
    if( db_task.full() ) {  
        while( !db_task.collect() ) ;  
    }
    to_dsa ++ ;
    db_task.add_op( DSA_OPCODE_COMPVAL , (void*)(uintptr_t)pattern , src , len ) ;
    return true ;
}

bool DSAbatch::submit_memfill( void *dest , uint64_t pattern , size_t len ) noexcept ( true ){
    if( db_task.full() ) {  
        while( !db_task.collect() ) ;  
    } 
    #ifdef BUILD_RELEASE
        if( len < 0x200 ) { 
            to_cpu ++ ;
            memfill_cpu( dest , pattern , len , _FLAG_CC_ ? 0 : 1 ) ; 
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
    db_task.add_op( DSA_OPCODE_MEMFILL , (void*)(dest_) , (void*)(uintptr_t)pattern , len ) ;
    #if defined(DESCS_ADDRESS_ALIGNMENT) and !defined(FLAG_CACHE_CONTROL)
        _mm_clflushopt( dest ) ; // persistent in memory
    #endif
    return true ;
}

bool DSAbatch::submit_memcpy( void *dest , const void* src , size_t len ) noexcept( true ){ 
    if( db_task.full() ) { 
        // memcpy( dest , src , len ) ; 
        // return true ; 
        while( !db_task.collect() ) ; 
        // return false ;
    } 
    // #ifdef BUILD_RELEASE
        if( len < 0x200 ) { 
            to_cpu ++ ;
            memmove_cpu( dest , src , len , _FLAG_CC_ ? 0 : 1 ) ;
            return true ;
        }
    // #endif 
    uintptr_t dest_ = (uintptr_t) dest , src_ = (uintptr_t)src ;
    #ifdef DESCS_ADDRESS_ALIGNMENT
        size_t align64_diff = 0 ;
        if( dest_ & 0x3f ){  
            align64_diff = 0x40 - ( dest_ & 0x3f ) ;
            align64_diff = len < align64_diff ? len : align64_diff ;
            memcpy( (void*)dest_ , (void*)src_ , align64_diff ) ;
            len -= align64_diff ;
            dest_ += align64_diff ;
            src_ += align64_diff ;  
        } 
    #endif
    to_dsa ++ ; 
    db_task.add_op( DSA_OPCODE_MEMMOVE , (void*)(dest_) , (void*)(src_) , len ) ;
    #if defined(DESCS_ADDRESS_ALIGNMENT) and !defined(FLAG_CACHE_CONTROL)
        _mm_clflushopt( dest ) ; // persistent in memory
    #endif 
    return true ;
}

bool DSAbatch::submit_flush( void *dest , size_t len ) noexcept( true ){
    if( db_task.full() ) { 
        while( !db_task.collect() ) ; // this op must use DSA
        // return false ;
    }     
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
    db_task.add_op( DSA_OPCODE_CFLUSH , (void*)(dest_) , len ) ;
    return true ;
}

bool DSAbatch::check() {
    return db_task.check() ;
}

void DSAbatch::wait(){
    db_task.wait() ; 
} 