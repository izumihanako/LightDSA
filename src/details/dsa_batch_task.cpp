#include "dsa_batch_task.hpp"
#include "dsa_constant.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>

// public
DSAbatch_task::DSAbatch_task( int bsiz , int cap ){
    init( bsiz , cap ) ; 
}

DSAbatch_task::DSAbatch_task( int bsiz , int cap , DSAworkingqueue *wq ){
    init( bsiz , cap ) ; 
    set_wq( wq ) ; 
}

DSAbatch_task::~DSAbatch_task(){
    free_descs() ;
}

// private 
void DSAbatch_task::init( int bsiz , int cap ) {
    if( cap < 2 ) {
        printf( "batch capacity adjusted: %d -> 2\n" , cap ) ;
        cap = 2 ;
    }
    batch_siz = bsiz ;
    batch_capacity = cap ;
    desc_capacity = cap * batch_siz ; 
    descs = bdesc = nullptr ;
    comps = bcomp = nullptr ;
    q_front = 0 , q_submit = 0 , q_back = 0 , q_ecnt = 0 ;
    front_comp = nullptr ;
    wq_portal = nullptr ;
    working_queue = nullptr ;
}

void DSAbatch_task::prepare_desc( dsa_opcode op_type ){
    dsa_hw_desc *desc =  descs + q_back ;
    dsa_completion_record* comp = comps + q_back ;
    desc->opcode = op_type ;   
    switch ( desc->opcode ) {
    case DSA_OPCODE_NOOP :
        desc->src_addr = desc->dst_addr = desc->xfer_size = 0 ;
        desc->flags = DSA_NOOP_FLAG ;
        break ;   
    default :
        printf( "prepare_desc(type %s) Unimplemented\n" , dsa_op_str( op_type ) ) ;
        exit( 1 ) ;
    }
    comp->status = 0 ;
}

void DSAbatch_task::prepare_desc( dsa_opcode op_type , void *src , uint32_t len , uint32_t stride ){
    // printf( "prepare_desc(type %s, src(dst) %p, len %u) @ %d\n" , dsa_op_str( op_type ) , src , len , q_back ) ;
    dsa_hw_desc *desc =  descs + q_back ;
    dsa_completion_record* comp = comps + q_back ;
    desc->opcode = op_type ;
    desc->xfer_size = len ;
    switch ( desc->opcode ) {
    case DSA_OPCODE_TRANSL_FETCH :  // 10
        desc->region_stride = stride ; // @ bytes 48
        desc->flags = DSA_TRANSL_FETCH_FLAG ;
        if( stride ) desc->flags |= ( 1 << 16 ) ;
        desc->src_addr = (uintptr_t) src ; 
        break ;
    case DSA_OPCODE_CFLUSH :        // 32
        desc->src_addr = 0 ;
        desc->dst_addr = (uintptr_t) src ;
        desc->flags = DSA_CFLUSH_FLAG ;
        break ;
    default:
        printf( "prepare_desc(type %s, src %p, len %u, stride %u) Unimplemented\n" , 
                dsa_op_str( op_type ) , src , len , stride ) ;
        exit( 1 ) ; 
    }
    comp->status = 0 ;
}

void DSAbatch_task::prepare_desc( dsa_opcode op_type , void *dest , const void* src , uint32_t len ){
    // printf( "prepare_desc(type %s, dest %p, src %p, len %u) @ %d\n" , dsa_op_str( op_type ) , dest , src , len , q_back ) ;
    dsa_hw_desc *desc =  descs + q_back ;
    dsa_completion_record* comp = comps + q_back ;
    desc->opcode = op_type ;
    desc->xfer_size = len ;
    switch ( desc->opcode ) {
    case DSA_OPCODE_MEMMOVE : // 3
        desc->src_addr  = (uintptr_t) src ;       // @ bytes 16
        desc->dst_addr  = (uintptr_t) dest ;      // @ bytes 24
        desc->flags     = DSA_MEMMOVE_FLAG ;
        break ;
    case DSA_OPCODE_MEMFILL : // 4 
        desc->pattern_lower = (uint64_t) src ;    // @ bytes 16
        desc->dst_addr  = (uintptr_t) dest ;      // @ bytes 24 
        desc->flags     = DSA_MEMFILL_FLAG ;
        break ;
    case DSA_OPCODE_COMPARE : // 5
        desc->src_addr  = (uintptr_t) src ;       // @ bytes 16
        desc->src2_addr = (uintptr_t) dest ;      // @ bytes 24
        desc->flags     = DSA_COMPARE_FLAG ;
        break;   
    case DSA_OPCODE_COMPVAL : // 6 
        desc->src_addr  = (uintptr_t) src ;       // @ bytes 16
        desc->comp_pattern = (uint64_t) dest ;    // @ bytes 24
        desc->flags     = DSA_COMPVAL_FLAG ;
        break;   
    default:
        printf( "prepare_desc(type %s, dest %p, src %p, len %u) Unimplemented\n" , 
                dsa_op_str( op_type ) , dest , src , len ) ;
        exit( 1 ) ; 
    }
    comp->status = 0 ;
}

void DSAbatch_task::alloc_descs( int cap ){
    if( cap < 2 ){
        printf( "DSA batch capacity needs to be greater than 1") ;
        exit( 1 ) ;
    }
    batch_capacity = cap ;
    desc_capacity = cap * batch_siz ; 
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        descs = ( dsa_hw_desc *) working_queue->allocator->allocate( desc_capacity * 64 ) ; // sizeof( desc ) = 64
        comps = ( dsa_completion_record *) working_queue->allocator->allocate( desc_capacity * 32 ) ; // sizeof( comp ) = 32
        bdesc = ( dsa_hw_desc *) working_queue->allocator->allocate( batch_capacity * 64 ) ; // sizeof( desc ) = 64
        bcomp = ( dsa_completion_record *) working_queue->allocator->allocate( batch_capacity * 32 ) ; // sizeof( comp ) = 32
    #else 
        descs = ( dsa_hw_desc *) aligned_alloc( 64 , desc_capacity * 64 ) ; // sizeof( desc ) = 64
        comps = ( dsa_completion_record *) aligned_alloc( 32 , desc_capacity * 32 ) ; // sizeof( comp ) = 32 
        bdesc = ( dsa_hw_desc *) aligned_alloc( 64 , batch_capacity * 64 ) ; // sizeof( desc ) = 64 
        bcomp = ( dsa_completion_record *) aligned_alloc( 32 , batch_capacity * 32 ) ; // sizeof( comp ) = 32 
    #endif
    memset( descs , 0 , desc_capacity * 64 ) ;
    memset( comps , 0 , desc_capacity * 32 ) ;
    for( int i = 0 ; i < desc_capacity ; i ++ ){ 
        descs[i].completion_addr = (uintptr_t)&comps[i] ;
    }
    memset( bdesc , 0 , batch_capacity * 64 ) ;
    memset( bcomp , 0 , batch_capacity * 32 ) ;
    for( int i = 0 ; i < batch_capacity ; i ++ ){  
        bdesc[i].flags = IDXD_OP_FLAG_CRAV | IDXD_OP_FLAG_RCR ;
        bdesc[i].completion_addr = (uintptr_t)&bcomp[i] ;
        bdesc[i].opcode = DSA_OPCODE_BATCH ;
    } 
    front_comp = &bcomp[0] ;
}

void DSAbatch_task::free_descs(){
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        if( working_queue == nullptr ) return ;
        if(descs) working_queue->allocator->deallocate( descs ) ;
        if(comps) working_queue->allocator->deallocate( comps ) ;
        if(bdesc) working_queue->allocator->deallocate( bdesc ) ; 
        if(bcomp) working_queue->allocator->deallocate( bcomp ) ; 
    #else 
        if(descs) free( descs ) ;
        if(comps) free( comps ) ; 
        if(bdesc) free( bdesc ) ; 
        if(bcomp) free( bcomp ) ; 
    #endif
}

void DSAbatch_task::clear(){  
    q_front = 0 , q_back = 0 , q_ecnt = 0 , q_submit = 0 ;
    if( front_comp && bcomp ) front_comp = &bcomp[0] ;
    // back always point to a empty slot
    // front always point to something( or nothing if empty ) 
}

void DSAbatch_task::add_no_op(){ 
    prepare_desc( DSA_OPCODE_NOOP ) ;
    q_back = ( q_back + 1 == desc_capacity ? 0 : q_back + 1 ) ; 
    q_ecnt ++ ;
}

void DSAbatch_task::submit_forward(){
    // printf( "submit_forward(): fr:%d  submit:%d  ba:%d  q_ecnt:%d\n" , q_front , q_submit , q_back , q_ecnt ) ; fflush( stdout ) ;
    do_op( q_submit ) ; 
    q_submit = ( q_submit + batch_siz == desc_capacity ? 0 : q_submit + batch_siz ) ; 
}

void DSAbatch_task::PF_adjust_desc( int desc_idx ){
    dsa_hw_desc *desc = &descs[desc_idx] ;
    dsa_completion_record *comp = &comps[desc_idx] ;
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
}

void DSAbatch_task::check_front() {
    // printf( "check_op(): fr:%d  submit:%d  ba:%d\n" , q_front , q_submit , q_back ) ; fflush( stdout ) ;
    switch ( op_status( front_comp->status ) ){ 
    case DSA_COMP_BATCH_PAGE_FAULT :
        printf( "DSA batch op page fault(6) occurred\n" ) ;
        solve_pf( q_front / batch_siz ) ;
        do_op( q_front ) ;
        break ;
    default:  { 
        printf( "DSA batch op error code 0x%x, %s\n" , front_comp->status , dsa_comp_status_str( front_comp->status ) ) ; 
        for( int i = q_front ; i < q_front + batch_siz ; i ++ ){
            uint8_t status = op_status( comps[i].status ) ;
            printf( "desc %d, status = 0x%x, %s\n" , i , status , dsa_comp_status_str( status ) ) ;
            if( status == DSA_COMP_SUCCESS ){
                // already done, set to no op
                descs[i].opcode = DSA_OPCODE_NOOP ;
                descs[i].flags = IDXD_OP_FLAG_CRAV | IDXD_OP_FLAG_RCR ;
                descs[i].src_addr = descs[i].dst_addr = descs[i].xfer_size = 0 ;
                comps[i].status = 0 ; 
            } else if( status == DSA_COMP_PAGE_FAULT_NOBOF ){
                int wr = comps[i].status & DSA_COMP_STATUS_WRITE ;
                volatile char *t = (char*) comps[i].fault_addr ;
                wr ? *t = *t : *t ; 
                PF_adjust_desc( i ) ;
            }
        }
        getchar() ;
        // re-submit
        do_op( q_front ) ;
        }
    } 
}

void DSAbatch_task::do_op( int desc_idx ) { 
    // printf( "do_op(): fr:%d  submit:%d  ba:%d\n" , q_front , q_submit , q_back ) ; fflush( stdout ) ;
    int batch_idx = desc_idx / batch_siz ;
    bcomp[batch_idx].status = 0 ;
    bdesc[batch_idx].desc_count = batch_siz ;
    bdesc[batch_idx].desc_list_addr = (uintptr_t) &descs[desc_idx] ;
    submit_desc( wq_portal , ACCFG_WQ_SHARED , &bdesc[batch_idx] ) ; 
}

void DSAbatch_task::solve_pf( int batch_idx ){
    puts( "solve pf" ) ;
    dsa_hw_desc &thisbdesc = bdesc[batch_idx] ;
    dsa_completion_record &thisbcomp = bcomp[batch_idx] ;
    int wr = thisbcomp.status & DSA_COMP_STATUS_WRITE ;
    volatile char *t = (char *)( thisbcomp.fault_addr ) ;
    wr ? *t = *t : *t ;  
    thisbdesc.desc_list_addr += thisbcomp.descs_completed * 64 ;
    thisbdesc.desc_count -= thisbcomp.descs_completed ;
    if( thisbdesc.desc_count == 1 ){
        thisbdesc.desc_count ++ ;
        thisbdesc.desc_list_addr += 64 ;
    }
    thisbcomp.status = 0 ;
} 


// public
bool DSAbatch_task::set_wq( DSAworkingqueue *new_wq ){ 
    if( new_wq == NULL ) return false ;
    if( working_queue != nullptr && check() == false ) {
        printf( "DSAbatch_task::set_wq() : task not finished\n" ) ;
        return false ;
    }
    free_descs() ; 
    working_queue = new_wq ;
    wq_portal = working_queue->get_portal() ;
    alloc_descs( batch_capacity ) ; 
    return true ;
}

void DSAbatch_task::add_op( dsa_opcode op_type ){
    // printf( "add_op(): fr:%d  submit:%d  ba:%d\n" , q_front , q_submit , q_back ) ; fflush( stdout ) ;
    prepare_desc( op_type ) ;   
    q_back = ( q_back + 1 == desc_capacity ? 0 : q_back + 1 ) ;
    q_ecnt ++ ;
    if( add_1_cap( q_back - q_submit ) >= batch_siz ) submit_forward() ; 
}

void DSAbatch_task::add_op( dsa_opcode op_type , void *dest , size_t len ){
    // printf( "add_op(): fr:%d  submit:%d  ba:%d\n" , q_front , q_submit , q_back ) ; fflush( stdout ) ;
    prepare_desc( op_type , dest , len ) ;   
    q_back = ( q_back + 1 == desc_capacity ? 0 : q_back + 1 ) ;
    q_ecnt ++ ;
    if( add_1_cap( q_back - q_submit ) >= batch_siz ) submit_forward() ; 
}

void DSAbatch_task::add_op( dsa_opcode op_type , void *dest , const void* src , size_t len ){
    // printf( "add_op(): fr:%d  submit:%d  ba:%d\n" , q_front , q_submit , q_back ) ; fflush( stdout ) ;
    prepare_desc( op_type , dest , src , len ) ;   
    q_back = ( q_back + 1 == desc_capacity ? 0 : q_back + 1 ) ;
    q_ecnt ++ ;
    if( add_1_cap( q_back - q_submit ) >= batch_siz ) submit_forward() ; 
}

void DSAbatch_task::wait(){
    while( q_back % batch_siz != 0 ) add_no_op() ;
    while( q_submit != q_back ) submit_forward() ; 
    while( q_ecnt ) { collect() ; }
}

bool DSAbatch_task::check(){
    while( q_back % batch_siz != 0 ) add_no_op() ;
    while( q_submit != q_back ) submit_forward() ;
    collect() ;
    return empty() ;
}

dsa_completion_record* DSAbatch_task::get_front_comp(){
    return front_comp ;
}

bool DSAbatch_task::collect(){ 
    // printf( "collect: fr:%d  submit:%d  ba:%d  ecnt:%d\n" , q_front , q_submit , q_back , q_ecnt ) ; fflush( stdout ) ; getchar() ;
    while( q_submit != q_front || q_ecnt == desc_capacity ){
        int x = op_status( front_comp->status ) ;  
        if( x == 1 ){ // success
            q_front += batch_siz , front_comp ++ ;
            if( q_front == desc_capacity ){
                q_front = 0 ; 
                front_comp = &bcomp[0] ;
            }
            q_ecnt -= batch_siz ;  
            continue ;
        } 
        if( x == 0 ) { 
            break ; // not finish yet
        }
        // some error occurred
        check_front() ; 
    }
    return q_ecnt < desc_capacity ;
}
