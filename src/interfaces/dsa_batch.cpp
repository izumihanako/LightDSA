#include "dsa_batch.hpp"
#include "details/dsa_agent.hpp" 
#include <cstring>
 
DSAbatch::DSAbatch( int bsiz , int cap ):db_task( bsiz , cap ) {
    db_task.set_port( DSAagent::get_instance().get_portal() ) ;
    cnt = 0 ; 
}

bool DSAbatch::submit_comp_pattern( void *src , uint64_t pattern , size_t len ) noexcept( true ){
    if( db_task.full() && !db_task.collect() ) {  
        while( !db_task.collect() ) ;  
    }
    cnt ++ ;
    db_task.add_op( DSA_OPCODE_COMPVAL , (void*)(uintptr_t)pattern , src , len ) ;
    return true ;
}

bool DSAbatch::submit_memfill( void *dest , uint64_t pattern , size_t len ) noexcept ( true ){
    if( db_task.full() && !db_task.collect() ) {  
        while( !db_task.collect() ) ;  
    } 
    #ifdef BUILD_RELEASE
        if( len < 0x400 ) { 
            int idx = 0 ;
            for( ; idx + 8 <= len ; idx += 8 ) {
                *(uint64_t*)( (uintptr_t)dest + idx ) = pattern ;
            } 
            if (len % 8 != 0) {
                uint64_t tmp_pattern = pattern;
                for (size_t i = 0; i < len % 8; ++i) {
                    *(uint8_t*)((uintptr_t)dest + idx + i) = (tmp_pattern >> (i * 8)) & 0xFF;
                }
            }
            return true ;
        }
    #endif
    uintptr_t dest_ = (uintptr_t) dest ;
    int align64_diff = 0 ;
    #ifdef DESCS_ADDRESS_ALIGNMENT
        if( dest_ & 0x3f ){  
            align64_diff = 0x40 - ( dest_ & 0x3f ) ;
            align64_diff = len < align64_diff ? len : align64_diff ;
            for( int i = 0 ; i + 8 <= align64_diff ; i += 8 )
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
    cnt ++ ; 
    db_task.add_op( DSA_OPCODE_MEMFILL , (void*)(dest_ + align64_diff) , (void*)(uintptr_t)pattern , len ) ;
    return true ;
}

bool DSAbatch::submit_memcpy( void *dest , const void* src , size_t len ) noexcept( true ){ 
    if( db_task.full() && !db_task.collect() ) { 
        // memcpy( dest , src , len ) ; 
        // return true ; 
        while( !db_task.collect() ) ; 
        // return false ;
    } 
    #ifdef BUILD_RELEASE
        if( len < 0x400 ) { 
            memcpy( dest , src , len ) ;
            return true ;
        }
    #endif 
    uintptr_t dest_ = (uintptr_t) dest , src_ = (uintptr_t)src ;
    int align64_diff = 0 ;
    #ifdef DESCS_ADDRESS_ALIGNMENT
        if( dest_ & 0x3f ){ 
            // printf( "src %p,  dest %p\n" , src_ , dest_ ) ;
            // getchar() ;
            align64_diff = 0x40 - ( dest_ & 0x3f ) ;
            align64_diff = len < align64_diff ? len : align64_diff ;
            memcpy( (void*)dest_ , (void*)src_ , align64_diff ) ;
            len -= align64_diff ;
            dest_ += align64_diff ;
            src_ += align64_diff ;  
        } 
    #endif
    cnt ++ ; 
    db_task.add_op( DSA_OPCODE_MEMMOVE , (void*)(dest_ + align64_diff) , (void*)(src_ + align64_diff) , len ) ;
    #if defined(DESCS_ADDRESS_ALIGNMENT) and !defined(FLAG_CACHE_CONTROL) and defined(FLAG_DEST_READBACK)
        _mm_clflushopt( dest ) ; // persistent in memory
    #endif 
    return true ;
}

bool DSAbatch::submit_flush( void *dest , size_t len ) noexcept( true ){
    if( db_task.full() && !db_task.collect() ) { 
        while( !db_task.collect() ) ; // this op must use DSA
        // return false ;
    }     
    uintptr_t dest_ = (uintptr_t) dest ; 
    #ifdef DESCS_ADDRESS_ALIGNMENT 
        if( dest_ & 0x3f ){
            len += ( dest_ & 0x3f ) ;
            dest_ &= ( ~0x3f ) ; 
        }
        if( len & 0x3f ){
            len += 0x40 - ( len & 0x3f ) ;
        }
    #endif
    cnt ++ ;
    db_task.add_op( DSA_OPCODE_CFLUSH , (void*)(dest_) , len ) ;
    return true ;
}

bool DSAbatch::check() {
    return db_task.check() ;
}

void DSAbatch::wait(){
    db_task.wait() ; 
} 