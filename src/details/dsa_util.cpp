#include <cstdio>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include "dsa_util.hpp" 

const char* dsa_op_str( uint32_t opcode ){ 
    #define PROCESS_VAL(p) case(p): return #p;
        switch( opcode ){	
            PROCESS_VAL( DSA_OPCODE_NOOP ); 
            PROCESS_VAL( DSA_OPCODE_BATCH ); 
            PROCESS_VAL( DSA_OPCODE_DRAIN ); 
            PROCESS_VAL( DSA_OPCODE_MEMMOVE ); 
            PROCESS_VAL( DSA_OPCODE_MEMFILL ); 
            PROCESS_VAL( DSA_OPCODE_COMPARE ); 
            PROCESS_VAL( DSA_OPCODE_COMPVAL ); 
            PROCESS_VAL( DSA_OPCODE_CR_DELTA ); 
            PROCESS_VAL( DSA_OPCODE_AP_DELTA ); 
            PROCESS_VAL( DSA_OPCODE_DUALCAST ); 
            PROCESS_VAL( DSA_OPCODE_TRANSL_FETCH ); 
            PROCESS_VAL( DSA_OPCODE_CRCGEN ); 
            PROCESS_VAL( DSA_OPCODE_COPY_CRC ); 
            PROCESS_VAL( DSA_OPCODE_DIF_CHECK ); 
            PROCESS_VAL( DSA_OPCODE_DIF_INS ); 
            PROCESS_VAL( DSA_OPCODE_DIF_STRP ); 
            PROCESS_VAL( DSA_OPCODE_DIF_UPDT ); 
            PROCESS_VAL( DSA_OPCODE_DIX_GEN ); 
            PROCESS_VAL( DSA_OPCODE_CFLUSH );
        }
    return "Invalid opcode" ;
    #undef PROCESS_VAL 
}

const char* dsa_comp_status_str( uint32_t opcode ){ 
    #define PROCESS_VAL(p) case(p): return #p;
        switch( opcode ){
            PROCESS_VAL( DSA_COMP_NONE ) ;
            PROCESS_VAL( DSA_COMP_SUCCESS ) ;
            PROCESS_VAL( DSA_COMP_SUCCESS_PRED ) ;
            PROCESS_VAL( DSA_COMP_PAGE_FAULT_NOBOF ) ;
            PROCESS_VAL( DSA_COMP_PAGE_FAULT_IR ) ;
            PROCESS_VAL( DSA_COMP_BATCH_FAIL ) ;
            PROCESS_VAL( DSA_COMP_BATCH_PAGE_FAULT ) ;
            PROCESS_VAL( DSA_COMP_DR_OFFSET_NOINC ) ;
            PROCESS_VAL( DSA_COMP_DR_OFFSET_ERANGE ) ;
            PROCESS_VAL( DSA_COMP_DIF_ERR ) ;
            PROCESS_VAL( DSA_COMP_BAD_OPCODE ) ;
            PROCESS_VAL( DSA_COMP_INVALID_FLAGS ) ;
            PROCESS_VAL( DSA_COMP_NOZERO_RESERVE ) ;
            PROCESS_VAL( DSA_COMP_XFER_ERANGE ) ;
            PROCESS_VAL( DSA_COMP_DESC_CNT_ERANGE ) ;
            PROCESS_VAL( DSA_COMP_DR_ERANGE ) ;
            PROCESS_VAL( DSA_COMP_OVERLAP_BUFFERS ) ;
            PROCESS_VAL( DSA_COMP_DCAST_ERR ) ;
            PROCESS_VAL( DSA_COMP_DESCLIST_ALIGN ) ;
            PROCESS_VAL( DSA_COMP_INT_HANDLE_INVAL ) ;
            PROCESS_VAL( DSA_COMP_CRA_XLAT ) ;
            PROCESS_VAL( DSA_COMP_CRA_ALIGN ) ;
            PROCESS_VAL( DSA_COMP_ADDR_ALIGN ) ;
            PROCESS_VAL( DSA_COMP_PRIV_BAD ) ;
            PROCESS_VAL( DSA_COMP_TRAFFIC_CLASS_CONF ) ;
            PROCESS_VAL( DSA_COMP_PFAULT_RDBA ) ;
            PROCESS_VAL( DSA_COMP_HW_ERR1 ) ;
            PROCESS_VAL( DSA_COMP_HW_ERR_DRB ) ;
            PROCESS_VAL( DSA_COMP_TRANSLATION_FAIL ) ;
            PROCESS_VAL( DSA_COMP_DRAIN_EVL ) ;
            PROCESS_VAL( DSA_COMP_BATCH_EVL_ERR ) ;
        }
    return "Invalid comp status" ;
    #undef PROCESS_VAL 
}

int open_wq( accfg_wq *wq ){
    int fd , rc ;
    char path[PATH_MAX] ; 
    rc = accfg_wq_get_user_dev_path( wq , path , sizeof( path ) ) ; 
    if( rc ) return rc ;
    fd = open( path , O_RDWR ) ;
    if( fd < 0 ) return -1 ;
    return fd ;
}

void* wq_mmap( accfg_wq *wq ){
    int fd ;
    void* wq_reg ;
    fd = open_wq( wq ) ;
    wq_reg = mmap( NULL , 0x1000 , PROT_WRITE , MAP_SHARED | MAP_POPULATE , fd , 0 ) ;
    if( wq_reg == MAP_FAILED ){
        close( fd ) ;
        return NULL ;
    }
    close( fd ) ;
    return wq_reg ;
} 

int find_avail_wq_portal( void *& wq_portal , int wq_type ){
    accfg_device *device ;
    accfg_wq *wq ;
    accfg_ctx *ctx ;
    accfg_new( &ctx ) ;
    wq_portal = MAP_FAILED ;
    bool found = false ;
    accfg_device_foreach( ctx , device ){
        // Use accfg_device_(*) functions to select enabled devices 
        // based on name, numa node
        accfg_wq_foreach( device , wq ){
            int fd ;
            if( accfg_wq_get_type( wq ) != ACCFG_WQT_USER )
                continue ;
            if( accfg_wq_get_mode( wq ) != wq_type )
                continue ;
            if( accfg_wq_get_state( wq ) != ACCFG_WQ_ENABLED )
                continue ;
            if( ( fd = open_wq( wq ) ) < 0 )
                continue ;
            close( fd ) ;
            found = true ;
            break ;
        }
        if( found ) break ;
    }
    if( !found ) {
        accfg_unref( ctx ) ;
        return -1 ;
    }
    wq_portal = wq_mmap( wq ) ;
    accfg_unref( ctx ) ;
    if( wq_portal == NULL ) return -1 ;
    return 0 ;
}

void print_desc( dsa_hw_desc &desc ){
    switch ( desc.opcode ) {
    case DSA_OPCODE_NOOP :
        printf( "op = NOOP, comp_addr = %p\n" , (void*)desc.completion_addr ) ;
        break;
    case DSA_OPCODE_MEMMOVE :
        printf( "op = MEMMOVE, comp_addr = %p\n" , (void*)desc.completion_addr ) ;
        printf( "src = %p , dest = %p , len = %d\n" , (void*)desc.src_addr , (void*)desc.dst_addr , desc.xfer_size ) ;
        break;
    case DSA_OPCODE_MEMFILL :
        printf( "op = MEMFILL, comp_addr = %p\n" , (void*)desc.completion_addr ) ;
        printf( "pattern = %lx , dest = %p , len = %d\n" , desc.pattern_lower , (void*)desc.dst_addr , desc.xfer_size ) ;
        break;
    case DSA_OPCODE_COMPARE :
        printf( "op = COMPARE, comp_addr = %p\n" , (void*)desc.completion_addr ) ;
        printf( "src = %p , dest = %p , len = %d\n" , (void*)desc.src_addr , (void*)desc.src2_addr , desc.xfer_size ) ;
        break;
    case DSA_OPCODE_COMPVAL :
        printf( "op = COMPVAL, comp_addr = %p\n" , (void*)desc.completion_addr ) ;
        printf( "src = %p , pattern = %lx , len = %d\n" , (void*)desc.src_addr , desc.comp_pattern , desc.xfer_size ) ;
        break;
    default:
        printf( "op = %s, comp_addr = %p\n" , dsa_op_str( desc.opcode ) , (void*)desc.completion_addr ) ;
        break;
    } 
}