#include "dsa_constant.hpp"
#include "dsa_task.hpp"

#include <cstring>
#include <cstdarg>
#include <cstdio>

void DSAtask::init(){
    memset( &desc , 0 , sizeof( desc ) ) ;
    wq_portal = nullptr ;
    working_queue = nullptr ;
    is_doing_flag = false ; 
    comp = nullptr ;
    op_cnt = page_fault_cnt = page_fault_resolving = 0 ;
    op_bytes = 0 ; 
    last_fault_addr = nullptr ;
}

void DSAtask::free_comp(){ 
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        if( comp ) working_queue->allocator->deallocate( comp ) ;
    #else 
        if( comp ) free( comp ) ;
    #endif 
}

DSAtask::DSAtask(){
    init() ;  
}
 
DSAtask::DSAtask( DSAworkingqueue *wq ){
    init() ; 
    set_wq( wq ) ;
}

DSAtask::~DSAtask(){
    free_comp() ;
}

void DSAtask::prepare_desc( dsa_opcode op_type ){
    desc.opcode = op_type ;
    desc.flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV ;
    ori_xfersize = 0 ;
    switch ( desc.opcode ) {
    case DSA_OPCODE_NOOP :  // 0
        desc.src_addr = desc.dst_addr = desc.xfer_size = 0 ;
        break ;
    default:
        printf( "prepare_desc(type %s) Unimplemented\n" , dsa_op_str( op_type ) ) ;
        exit( 1 ) ;  
    }
    comp->status = 0 ; // DSA_COMP_NONE     
} 

void DSAtask::prepare_desc( dsa_opcode op_type , void *src , uint32_t len , uint32_t stride ){ 
    desc.opcode = op_type ;
    desc.region_size = len ;  // @ bytes 32, also xfer_size
    ori_xfersize = len ;
    switch ( desc.opcode ) {
    case DSA_OPCODE_TRANSL_FETCH :  // 10
        desc.region_stride = stride ; // @ bytes 48
        desc.flags = DSA_TRANSL_FETCH_FLAG ;
        if( stride ) desc.flags |= ( 1 << 16 ) ;
        desc.src_addr = (uintptr_t) src ;
        break ;
    case DSA_OPCODE_CFLUSH :        // 32
        desc.dst_addr = (uintptr_t) src ;  
        desc.flags = DSA_CFLUSH_FLAG ;
        if( stride ) printf( "prepare_desc(type %s, src %p, len %u, stride %u) stride ignored\n" , 
            dsa_op_str( op_type ) , src , len , stride ) ;
        break ;
    default:
        printf( "prepare_desc(type %s, src %p, len %u, stride %u) Unimplemented\n" , 
            dsa_op_str( op_type ) , src , len , stride ) ;
        exit( 1 ) ;  
        break;
    }
    comp->status = 0 ; // DSA_COMP_NONE     
} 

void DSAtask::prepare_desc( dsa_opcode op_type , const void *dest , const void* src , uint32_t len ){ 
    desc.opcode = op_type ;
    desc.xfer_size = len ;  // @ bytes 32  
    ori_xfersize = len ;
    switch ( desc.opcode ) {
    case DSA_OPCODE_MEMMOVE : // 3
        desc.src_addr  = (uintptr_t) src ;       // @ bytes 16
        desc.dst_addr  = (uintptr_t) dest ;      // @ bytes 24
        desc.flags     = DSA_MEMMOVE_FLAG ;
        break ;
    case DSA_OPCODE_MEMFILL : // 4 
        desc.pattern_lower = (uint64_t) src ;    // @ bytes 16
        desc.dst_addr  = (uintptr_t) dest ;      // @ bytes 24
        desc.flags     = DSA_MEMFILL_FLAG ;
        break ;
    case DSA_OPCODE_COMPARE : // 5
        desc.src_addr  = (uintptr_t) src ;       // @ bytes 16
        desc.src2_addr = (uintptr_t) dest ;      // @ bytes 24
        desc.flags     = DSA_COMPARE_FLAG ;
        break ;
    case DSA_OPCODE_COMPVAL : // 6 
        desc.src_addr  = (uintptr_t) src ;       // @ bytes 16
        desc.comp_pattern = (uint64_t) dest ;    // @ bytes 24
        desc.flags     = DSA_COMPVAL_FLAG ;
        break ;   
    default:        
        printf( "prepare_desc(type %s, dest %p, src %p, len %u) Unimplemented\n" , 
            dsa_op_str( op_type ) , dest , src , len ) ;
        exit( 1 ) ;  
        break ;
    }
    comp->status = 0 ; // DSA_COMP_NONE     
}

void DSAtask::PF_adjust_desc(){
    desc.xfer_size -= comp->bytes_completed ;
    switch ( desc.opcode ) {
    case DSA_OPCODE_MEMMOVE : // 3 
        desc.src_addr += comp->bytes_completed ;
        desc.dst_addr += comp->bytes_completed ;
        break ;
    case DSA_OPCODE_MEMFILL : // 4
        desc.dst_addr += comp->bytes_completed ;
        break; 
    case DSA_OPCODE_COMPARE : // 5
        desc.src_addr += comp->bytes_completed ;
        desc.src2_addr += comp->bytes_completed ;
        break ;
    case DSA_OPCODE_COMPVAL : // 6
        desc.src_addr += comp->bytes_completed ;
        break ;
    case DSA_OPCODE_CFLUSH  : // 32
        desc.dst_addr += comp->bytes_completed ;
        break ;
    default:
        printf( "%s page fault handler unimplemented" , dsa_op_str( desc.opcode ) ) ;
        exit( 1 ) ;
        break;
    }        
}

// public 

void DSAtask::set_wq( DSAworkingqueue *new_wq ){
    if( new_wq == nullptr ) return ;
    free_comp() ;
    working_queue = new_wq ;
    wq_portal = working_queue->get_portal() ;
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        comp = (dsa_completion_record*) working_queue->allocator->allocate( sizeof( dsa_completion_record ) ) ;
    #else 
        comp = (dsa_completion_record*) aligned_alloc( 4096 , sizeof( dsa_completion_record ) ) ;
    #endif
    memset( comp , 0 , sizeof( dsa_completion_record ) ) ;
    comp->status = 0 ; // DSA_COMP_NONE
    desc.completion_addr = (uintptr_t) comp ; // @ bytes 8
}

void DSAtask::solve_pf(){
    int wr = comp->status & DSA_COMP_STATUS_WRITE ;
    volatile char *t = (char*) comp->fault_addr ;
    wr ? *t = *t : *t ; 
    PF_adjust_desc() ;
    #if defined( INTERLEAVED_PAGEFAULT_ENABLE ) 
        char* this_fault = (char*) comp->fault_addr ;
        if( last_fault_addr && this_fault - last_fault_addr < DSA_PF_LEN_LIMIT ){ 
            page_fault_resolving ++ ;
            int len = desc.xfer_size > DSA_PAGE_FAULT_TOUCH_LEN ? DSA_PAGE_FAULT_TOUCH_LEN : desc.xfer_size ;
            touch_trigger_pf( this_fault , len , wr ) ;
        }
        last_fault_addr = this_fault ;
    #endif
    comp->status = 0 ; // reset comp->status, or if will be triggered repeatedly
} 

bool DSAtask::check(){
    if( is_doing_flag == false ) return true ;

    volatile uint8_t &status = comp->status ;
    switch ( op_status( status ) ) {
    case DSA_COMP_SUCCESS :
        is_doing_flag = false ;
        return true ;
    case DSA_COMP_NONE :
        break ;
    case DSA_COMP_PAGE_FAULT_NOBOF :
        // printf( "DSA op page fault(3) occurred\n" ) ;
        page_fault_cnt ++ ;
        solve_pf() ; 
        do_op() ;
        break ;
    default:
        printf( "DSA op error code 0x%x, %s\n" , op_status( status ), dsa_comp_status_str( op_status( status ) ) ) ;
        exit( -1 ) ;
    }
    return false ;
}

void DSAtask::do_op() noexcept( true ) {
    is_doing_flag = true ; 
    #ifdef INTERLEAVED_PAGEFAULT_ENABLE 
        if( desc.xfer_size > 128 * KB ){
            switch ( desc.opcode ) { 
                case DSA_OPCODE_MEMMOVE : // 3
                    touch_trigger_pf( (char*) desc.src_addr , 128 * KB , 0 ) ;
                    touch_trigger_pf( (char*) desc.dst_addr , 128 * KB , 1 ) ; 
                    break; 
                case DSA_OPCODE_MEMFILL : // 4
                    touch_trigger_pf( (char*) desc.dst_addr , 128 * KB , 1 ) ; 
                    break;
                case DSA_OPCODE_COMPARE : // 5
                    touch_trigger_pf( (char*) desc.src_addr , 128 * KB , 0 ) ;
                    touch_trigger_pf( (char*) desc.src2_addr , 128 * KB , 0 ) ;
                    break;
                case DSA_OPCODE_COMPVAL : // 6
                    touch_trigger_pf( (char*) desc.src_addr , 128 * KB , 0 ) ;
                    break;
                default:
                    break;
            } 
        }
    #endif
    last_fault_addr = nullptr ;
    submit_desc( wq_portal , ACCFG_WQ_SHARED , &desc ) ; // 提交之后就开始DSA操作了
    op_cnt ++ ;
    op_bytes += desc.xfer_size ;
}  

volatile uint8_t &DSAtask::comp_status_ref(){ return (comp->status) ; }

volatile uint8_t *DSAtask::comp_status_ptr(){ return &(comp->status) ; }

void DSAtask::print_stats(){
    printf( "DSA op stats : %d ops, %lu bytes, %d page faults\n" , op_cnt , op_bytes , page_fault_cnt ) ;
    // printf( "             : last opcode %s\n" , dsa_op_str( desc.opcode ) ) ;
    // printf( "             : last status %s\n" , dsa_comp_status_str( comp->status ) ) ;
}