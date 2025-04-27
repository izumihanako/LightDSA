#include "dsa_cpupath.hpp"
#include "dsa_batch_redistribute.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <x86intrin.h>

void memmove_cpu( void* dest , const void* src , size_t len , bool flush ){
    memmove( dest , src , len ) ;
    if( flush ){
        for( size_t i = 0 ; i < len ; i += 64 )
            _mm_clflushopt( (char*)((uintptr_t)dest + i) ) ;
        // _mm_sfence() ; 
    }
}

void memfill_cpu( void* dest , uint64_t pattern , size_t len , bool flush ){
    uint64_t *ptr = (uint64_t*)dest ;
    size_t i = 0 ;
    for( i = 0 ; i + 4 < len / 8 ; i += 4 ){
        ptr[i] = pattern ;
        ptr[i + 1] = pattern ;
        ptr[i + 2] = pattern ;
        ptr[i + 3] = pattern ;
    } 
    for( ; i < len / 8 ; i ++ ) ptr[i] = pattern ; 
    for( i = len / 8 * 8 ; i < len ; i ++ ) {
        *(uint8_t*)((uintptr_t)ptr + i) = (pattern >> (i * 8)) & 0xFF;
    }
    if( flush ){
        for( size_t i = 0 ; i < len ; i += 64 )
            _mm_clflushopt( (char*)((uintptr_t)dest + i) ) ;
        // _mm_sfence() ;
    }
}

int compare_cpu( void* dest , void* src , size_t len ){
    uint64_t *ptrA = (uint64_t*)dest ;
    uint64_t *ptrB = (uint64_t*)src ;
    size_t i = 0 ;
    for( ; i < len / 8 ; i ++ ){
        if( ptrA[i] != ptrB[i] ){
            for( int j = 0 ; j < 8 ; j ++ ){
                if( *(uint8_t*)((uintptr_t)ptrA + i * 8 + j) != *(uint8_t*)((uintptr_t)ptrB + i * 8 + j) ){
                    return i * 8 + j ;
                }
            }
        }
    }
    for( i = len / 8 * 8 ; i < len ; i ++ ){
        if( *(uint8_t*)((uintptr_t)ptrA + i) != *(uint8_t*)((uintptr_t)ptrB + i) ) return i ;
    }
    return len;
}

int compval_cpu( void* dest , uint64_t pattern , size_t len ){
    uint64_t *ptr = (uint64_t*)dest ;
    size_t i = 0 ;
    for( ; i < len / 8 ; i ++ ){
        if( ptr[i] != pattern ){
            for( int j = 0 ; j < 8 ; j ++ ){
                if( *(uint8_t*)((uintptr_t)ptr + i * 8 + j) != ( (pattern >> (j * 8)) & 0xFF ) ){
                    return i * 8 + j ;
                }
            }
        }
    }
    for( i = len / 8 * 8 ; i < len ; i ++ ){
        if( *(uint8_t*)((uintptr_t)ptr + i) != ( (pattern >> (i * 8)) & 0xFF ) ) return i ;
    }
    return len;
}

void flush_cpu( void* dest , size_t len ){
    for(size_t i = 0 ; i < len ; i += 8 * 64 )
        _mm_clflushopt( (char*)((uintptr_t)dest + i) ) ; 
}

void do_by_cpu( dsa_hw_desc *desc , dsa_completion_record *comp ){
    switch ( desc->opcode ) {
    case DSA_OPCODE_NOOP :
        break ;
    case DSA_OPCODE_MEMMOVE :
        memmove_cpu( (void*)desc->dst_addr , (void*)desc->src_addr , desc->xfer_size , ( desc->flags & IDXD_OP_FLAG_CC ) ? 0 : 1 ) ;
        if( comp ) comp->bytes_completed = desc->xfer_size ;
        break ;
    case DSA_OPCODE_MEMFILL :
        memfill_cpu( (void*)desc->dst_addr , desc->pattern_lower , desc->xfer_size , ( desc->flags & IDXD_OP_FLAG_CC ) ? 0 : 1 ) ;
        if( comp ) comp->bytes_completed = desc->xfer_size ;
        break ;
    case DSA_OPCODE_COMPARE :{
        int res = compare_cpu( (void*)desc->src_addr , (void*)desc->src2_addr , desc->xfer_size ) ;
        if( !comp ) break ;
        if( (uint32_t)res == desc->xfer_size ){
            comp->result = 0 ;
            comp->bytes_completed = 0 ;
        } else {
            comp->result = 1 ;
            comp->bytes_completed = res ;
        }
        break ;
    } 
    case DSA_OPCODE_COMPVAL :{
        int res = compval_cpu( (void*)desc->src_addr , desc->pattern_lower , desc->xfer_size ) ;
        if( !comp ) break ;
        if( (uint32_t)res == desc->xfer_size ){
            comp->result = 0 ;
            comp->bytes_completed = 0 ;
        } else {
            comp->result = 1 ;
            comp->bytes_completed = res ;
        }
        break ;
    } 
    case DSA_OPCODE_CFLUSH :
        flush_cpu( (void*)desc->dst_addr , desc->xfer_size ) ;
        _mm_sfence() ;
        break ;
    default:
        printf( "do_by_cpu() : Unimplemented\n" ) ;
        exit( 1 ) ;
    }
    if( comp ) comp->status = DSA_COMP_SUCCESS ;
}


void do_by_cpu( const dsa_rdstrb_entry &entry ){
    switch ( entry.opcode ) {
    case DSA_OPCODE_NOOP :
        break ;
    case DSA_OPCODE_MEMMOVE :
        memmove_cpu( (void*)entry.dst_addr , (void*)entry.src_addr , entry.xfer_size , ( entry.flags & IDXD_OP_FLAG_CC ) ? 0 : 1 ) ;
        break ;
    case DSA_OPCODE_MEMFILL :
        memfill_cpu( (void*)entry.dst_addr , entry.pattern_lower , entry.xfer_size , ( entry.flags & IDXD_OP_FLAG_CC ) ? 0 : 1 ) ;
        break ;
    case DSA_OPCODE_COMPARE :{
        int res = compare_cpu( (void*)entry.src_addr , (void*)entry.src2_addr , entry.xfer_size ) ;
        break ;
    } 
    case DSA_OPCODE_COMPVAL :{
        int res = compval_cpu( (void*)entry.src_addr , entry.pattern_lower , entry.xfer_size ) ; 
        break ;
    } 
    case DSA_OPCODE_CFLUSH :
        flush_cpu( (void*)entry.dst_addr , entry.xfer_size ) ;
        _mm_sfence() ;
        break ;
    default:
        printf( "do_by_cpu() : Unimplemented\n" ) ;
        exit( 1 ) ;
    } 
}