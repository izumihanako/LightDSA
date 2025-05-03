#include "dsa_pure_batch_task.hpp" 
#include "dsa_cpupath.hpp"  

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
  
// public
DSA_pure_batch_task::DSA_pure_batch_task( int bsiz ){
    init( bsiz ) ; 
}

DSA_pure_batch_task::DSA_pure_batch_task( int bsiz , DSAworkingqueue *wq ){
    init( bsiz ) ; 
    set_wq( wq ) ; 
}

DSA_pure_batch_task::~DSA_pure_batch_task(){
    free_descs() ;
}

// private 
void DSA_pure_batch_task::init( int bsiz ) { 
    batch_siz = bsiz , desc_cnt = 0 ;
    is_doing_flag = false ; 
    descs = bdesc = nullptr ;
    comps = bcomp = nullptr ; 
    wq_portal = nullptr ;
    working_queue = nullptr ;  
    // stats 
    page_fault_resolving = 0 ;
    batch_fail_cnt = 0 ;
    op_cnt = op_bytes = 0 ; 
}

void DSA_pure_batch_task::prepare_desc( int idx , const dsa_rdstrb_entry &entry ){
    dsa_hw_desc *desc =  descs + idx ;
    dsa_completion_record* comp = comps + idx ;
    desc->xfer_size = entry.xfer_size ;
    desc->opcode = entry.opcode ; 
    desc->src_addr = entry.src_addr ;
    desc->dst_addr = entry.dst_addr ; 
    desc->flags    = entry.flags ;
    comp->status = 0 ; 
}

void DSA_pure_batch_task::alloc_descs(){
    if( batch_siz < 2 ){
        printf( "DSA batch size needs to be greater than 1") ;
        exit( 1 ) ;
    } 
    descs = ( dsa_hw_desc *) aligned_alloc( 64 , batch_siz * 64 ) ; // sizeof( desc ) = 64
    comps = ( dsa_completion_record *) aligned_alloc( 32 , batch_siz * 32 ) ; // sizeof( comp ) = 32 
    bdesc = ( dsa_hw_desc *) aligned_alloc( 64 , 1 * 64 ) ;     // sizeof( desc ) = 64 
    bcomp = ( dsa_completion_record *) aligned_alloc( 32 , 1 * 32 ) ; // sizeof( comp ) = 32   
    if( descs == nullptr || comps == nullptr || bdesc == nullptr || bcomp == nullptr ){
        printf( "DSAbatch_task::alloc_descs() : memory allocation failed\n" ) ;
        exit( 1 ) ;
    }
    memset( descs , 0 , batch_siz * 64 ) ;
    memset( comps , 0 , batch_siz * 32 ) ;
    for( int i = 0 ; i < batch_siz ; i ++ ){ 
        descs[i].completion_addr = (uintptr_t)&comps[i] ;
    }
    memset( bdesc , 0 , 1 * 64 ) ;
    memset( bcomp , 0 , 1 * 32 ) ; 
    bdesc->flags = IDXD_OP_FLAG_CRAV | IDXD_OP_FLAG_RCR ;
    bdesc->completion_addr = (uintptr_t) bcomp ;
    bdesc->opcode = DSA_OPCODE_BATCH ; 
    bdesc->desc_count = batch_siz ;
    bdesc->desc_list_addr = (uintptr_t) &descs[0] ; 
}

void DSA_pure_batch_task::free_descs(){
    if(descs) free( descs ) ;
    if(comps) free( comps ) ; 
    if(bdesc) free( bdesc ) ; 
    if(bcomp) free( bcomp ) ;  
    descs = bdesc = nullptr ;
    comps = bcomp = nullptr ; 
}

void DSA_pure_batch_task::print_stats(){
    printf( "DSA_pure_batch_task, this class shuld be used only for experiments\n" ) ;
    printf( "             : batch_siz = %d , desc_cnt = %d\n" , batch_siz , desc_cnt ) ;
    printf( "             : op_cnt = %lld , op_bytes = %lld\n" , op_cnt , op_bytes ) ;
    printf( "             : page_fault_resolving = %d , batch_fail_cnt = %d\n" , page_fault_resolving , batch_fail_cnt ) ;
    printf( "             : wq = %s, " , working_queue->get_name() ) ;  
} 

void DSA_pure_batch_task::PF_adjust_desc( int idx ){
    dsa_hw_desc *desc = descs + idx ;
    dsa_completion_record *comp = comps + idx ;
    desc->xfer_size -= comp->bytes_completed ;
    switch ( desc->opcode ) {
    case DSA_OPCODE_MEMMOVE : // 3 
        desc->src_addr += comp->bytes_completed ;
        desc->dst_addr += comp->bytes_completed ;
        break ;
    case DSA_OPCODE_MEMFILL : // 4
        desc->dst_addr += comp->bytes_completed ;
        break; 
    case DSA_OPCODE_COMPARE : // 5
        desc->src_addr += comp->bytes_completed ;
        desc->src2_addr += comp->bytes_completed ;
        break ;
    case DSA_OPCODE_COMPVAL : // 6
        desc->src_addr += comp->bytes_completed ;
        break ;
    case DSA_OPCODE_CFLUSH  : // 32
        desc->dst_addr += comp->bytes_completed ;
        break ;
    default:
        printf( "%s page fault handler unimplemented" , dsa_op_str( desc->opcode ) ) ;
        exit( 1 ) ;
        break;
    }
    comp->status = 0 ;
}

void DSA_pure_batch_task::resolve_error() {
    int batch_idx = 0 ; 
    int x = op_status( bcomp->status ) ;
    if( x == 1 || x == 0 ) return ; // success or not finish yet
    printf( "DSA batch op error code 0x%x, %s\n" , x , dsa_comp_status_str( x ) ) ; fflush( stdout ) ;
    batch_fail_cnt ++ ;
    int batch_base = batch_idx * batch_siz ;
    for( int i = 0 ; i < batch_siz ; i ++ ){
        int idx = batch_base + i ;
        uint8_t status = op_status( comps[idx].status ) ;
        printf( "desc %d, status = 0x%x, %s\n" , idx , status , dsa_comp_status_str( status ) ) ;
        if( status == DSA_COMP_SUCCESS){
            if( descs[idx].opcode == DSA_OPCODE_NOOP ) continue ; // no op
            // already done, set to no op
            descs[idx].opcode = DSA_OPCODE_NOOP ;
            descs[idx].flags = DSA_NOOP_FLAG ;
            descs[idx].src_addr = descs[idx].dst_addr = descs[idx].xfer_size = 0 ;
            comps[idx].status = 0 ;
        } else if( status == DSA_COMP_PAGE_FAULT_NOBOF ){
            int wr = comps[idx].status & DSA_COMP_STATUS_WRITE ;
            volatile char *t = (char*) comps[idx].fault_addr ;
            wr ? *t = *t : *t ; 
            PF_adjust_desc( idx ) ; 
        } else {
            // not implemented yet
        }
    } 
    getchar() ;
}

void DSA_pure_batch_task::do_op() { 
    bcomp->status = 0 ;
    is_doing_flag = true ;
    bdesc->desc_count = desc_cnt ; 
    submit_desc( wq_portal , ACCFG_WQ_SHARED , bdesc ) ; 
}
 
// public
bool DSA_pure_batch_task::set_wq( DSAworkingqueue *new_wq ){ 
    if( new_wq == NULL ) return false ;
    if( working_queue != nullptr && check( true ) == false ) {
        printf( "DSAbatch_task::set_wq() : task not finished\n" ) ;
        return false ;
    }
    free_descs() ; 
    working_queue = new_wq ;
    wq_portal = working_queue->get_portal() ;
    alloc_descs() ; 
    return true ;
}

void DSA_pure_batch_task::add_op( dsa_opcode op_type ){ 
    op_cnt ++ ; 
    if( !is_pos_valid() ) {
        printf( "DSA_pure_batch_task::add_op() : full\n" ) ;
        return ;
    } 
    prepare_desc( now_pos() , dsa_rdstrb_entry( op_type ) ) ; 
    forward_pos() ; 
}

void DSA_pure_batch_task::add_op( dsa_opcode op_type , void *dest , size_t len ){
    op_cnt ++ ;
    op_bytes += len ;
    if( !is_pos_valid() ) {
        printf( "DSA_pure_batch_task::add_op() : full\n" ) ;
        return ;
    }  
    prepare_desc( now_pos() , dsa_rdstrb_entry( op_type , dest , len ) ) ;   
    forward_pos() ; 
}

void DSA_pure_batch_task::add_op( dsa_opcode op_type , void *dest , const void* src , size_t len ){
    op_cnt ++ ;
    op_bytes += len ;
    if( !is_pos_valid() ) {
        printf( "DSAbatch_task::add_op() : full\n" ) ;
        return ;
    }   
    prepare_desc( now_pos() , dsa_rdstrb_entry( op_type , dest , src , len ) ) ;
    forward_pos() ; 
}

void DSA_pure_batch_task::wait(){ 
    while( check( true ) == false ) { usleep( 20 ) ; }
}

bool DSA_pure_batch_task::check( bool do_resolve_error ){ 
    if( desc_cnt == 0 ) return true ;
    if( desc_cnt == 1 ) add_op( DSA_OPCODE_NOOP ) ;
    if( is_doing_flag == false ) do_op() ; 
    if( bcomp->status == 0 ) return false ; // not finished yet
    if( op_status( bcomp->status ) == 1 ) {
        is_doing_flag = false ;
        desc_cnt = 0 ;
        return true ; // success
    }
    if( do_resolve_error ){
        resolve_error() ;
        do_op() ;
    }
    return false ;
}
