#include "dsa_batch_task.hpp" 
#include "dsa_cpupath.hpp" 

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
 
void batch_record_queue::init( int cap ){
    queue_cap = cap ;
    queue = (int*) malloc( sizeof(int) * queue_cap ) ; 
    clear() ;
}

batch_record_queue::~batch_record_queue(){
    if( queue ) free( queue ) ;
}

void batch_record_queue::push_back( int x ){
    if( full() ) {
        printf( "batch_record_queue::push_back() : queue full\n" ) ;
        return ;
    }
    queue[q_back] = x ;
    q_back = ( q_back + 1 == queue_cap ? 0 : q_back + 1 ) ;
    q_ecnt ++ ;
}

void batch_record_queue::pop(){
    if( empty() ) {
        printf( "batch_record_queue::pop() : queue empty\n" ) ;
        return ;
    }
    q_front = ( q_front + 1 == queue_cap ? 0 : q_front + 1 ) ;
    q_ecnt -- ;
}

int batch_record_queue::pop_front(){
    if( empty() ) return -1 ;
    int tmp = queue[q_front] ;
    q_front = (q_front + 1 == queue_cap ? 0 : q_front + 1);
    q_ecnt -- ;  
    return tmp ;
} 

void batch_record_queue::delay_front_and_pop( int pos ){  
    queue[pos] = queue[q_front] ;
    q_front = (q_front + 1 == queue_cap ? 0 : q_front + 1);
    q_ecnt -- ; 
} 

void batch_record_queue::print() {
    printf_RGB( 0xff0000 , "batch_record_queue:: %d elements : " , q_ecnt ) ;
    bool inside = q_back >= q_front ? false : true ;
    for( int i = 0 ; i < queue_cap ; i ++ ){
        if( q_front == i ) printf_RGB( 0x00ff00 , "[" ) , inside ^= 1 ;
        if( q_back  == i ) printf_RGB( 0x00ff00 , "]" ) , inside ^= 1 ;
        if( inside ) printf_RGB( 0x00ff00 , "%d " , queue[i] ) ;
        else printf_RGB( 0xffffff , "%d " , queue[i] ) ;
    }
    printf("\n"); fflush( stdout ) ;
}

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
    batch_siz = bsiz , batch_capacity = cap ;
    desc_capacity = cap * batch_siz ; 
    descs = bdesc = nullptr ;
    comps = bcomp = nullptr ; 
    last_fault_addr = nullptr ;
    wq_portal = nullptr ;
    working_queue = nullptr ; 
    free_queue.init( batch_capacity + 1 ) ;
    buzy_queue.init( batch_capacity + 1 ) ; 
    rdstrb.init( bsiz ) ;
    // stats 
    page_fault_resolving = 0 ;
    batch_fail_cnt = 0 ;
    op_cnt = op_bytes = 0 ;
    clear() ;
}

void DSAbatch_task::prepare_desc( int idx , const dsa_rdstrb_entry &entry ){
    dsa_hw_desc *desc =  descs + idx ;
    dsa_completion_record* comp = comps + idx ;
    desc->opcode = entry.opcode ;
    desc->xfer_size = entry.xfer_size ;
    desc->src_addr = entry.src_addr ;
    desc->dst_addr = entry.dst_addr ; 
    desc->flags    = entry.flags ;
    comp->status = 0 ; 
    last_fault_addr[idx] = nullptr ;
}

void DSAbatch_task::alloc_descs(){
    if( batch_capacity < 1 ){
        printf( "DSA batch capacity needs to be greater than 0") ;
        exit( 1 ) ;
    }
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        descs = ( dsa_hw_desc *) working_queue->allocator->allocate( desc_capacity * 64 , 64 ) ; // sizeof( desc ) = 64
        // working_queue->allocator->print_space() ;
        comps = ( dsa_completion_record *) working_queue->allocator->allocate( desc_capacity * 32 , 32 ) ; // sizeof( comp ) = 32
        // working_queue->allocator->print_space() ;
        bdesc = ( dsa_hw_desc *) working_queue->allocator->allocate( batch_capacity * 64 , 64 ) ; // sizeof( desc ) = 64
        // working_queue->allocator->print_space() ;
        bcomp = ( dsa_completion_record *) working_queue->allocator->allocate( batch_capacity * 32 , 32 ) ; // sizeof( comp ) = 32
        // working_queue->allocator->print_space() ; 
    #else 
        descs = ( dsa_hw_desc *) aligned_alloc( 64 , desc_capacity * 64 ) ; // sizeof( desc ) = 64
        comps = ( dsa_completion_record *) aligned_alloc( 32 , desc_capacity * 32 ) ; // sizeof( comp ) = 32 
        bdesc = ( dsa_hw_desc *) aligned_alloc( 64 , batch_capacity * 64 ) ; // sizeof( desc ) = 64 
        bcomp = ( dsa_completion_record *) aligned_alloc( 32 , batch_capacity * 32 ) ; // sizeof( comp ) = 32 
    #endif
    last_fault_addr = (char**) malloc( sizeof(char*) * desc_capacity ) ;
    if( descs == nullptr || comps == nullptr || bdesc == nullptr || 
        bcomp == nullptr || last_fault_addr == nullptr ){
        printf( "DSAbatch_task::alloc_descs() : memory allocation failed\n" ) ;
        exit( 1 ) ;
    }
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
        bdesc[i].desc_count = batch_siz ;
        bdesc[i].desc_list_addr = (uintptr_t) &descs[i*batch_siz] ;
    } 
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
    if( last_fault_addr ) free( last_fault_addr ) ;
    descs = bdesc = nullptr ;
    comps = bcomp = nullptr ;
    last_fault_addr = nullptr ;
}

void DSAbatch_task::clear(){
    batch_idx = batch_base = desc_idx = 0 ;
    free_queue.clear() ;
    buzy_queue.clear() ; 
    // 0 is already in use
    for( int i = 1 ; i < batch_capacity ; i ++ ) free_queue.push_back( i ) ; 
}

void DSAbatch_task::print_stats(){
    printf( "DSAbatch_task: batch_capacity = %d , batch_siz = %d\n" , batch_capacity , batch_siz ) ;
    printf( "             : op_cnt = %lld , op_bytes = %lld\n" , op_cnt , op_bytes ) ;
    printf( "             : page_fault_resolving = %d , batch_fail_cnt = %d\n" , page_fault_resolving , batch_fail_cnt ) ;
    printf( "             : wq = %s, " , working_queue->get_name() ) ;
    #if defined( ALLOCATOR_CONTIGUOUS_ENABLE )
        printf( "allocator used = %.3f MB\n" , 1.0 * working_queue->allocator->get_used_size() / MB ) ;
    #else
        printf( "memory_used = %.3f MB\n" , 1.0 * ( desc_capacity * 96 + batch_capacity * 96 ) / MB ) ;
    #endif  
}

void DSAbatch_task::add_no_op(){
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        rdstrb.push_back( dsa_rdstrb_entry( DSA_OPCODE_NOOP ) ) ; 
    #else 
        prepare_desc( now_pos() , dsa_rdstrb_entry( DSA_OPCODE_NOOP ) ) ; 
        forward_pos() ; 
    #endif  
}

void DSAbatch_task::submit_forward(){ 
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        // int tmp ;
        // if( tmp = rdstrb.special_rule_for_all_small() ){
        //     op_cnt -= batch_siz ;
        //     op_bytes -= tmp ;
        //     return ;
        // } 
        for( int i = 0 ; i < batch_siz ; i ++ ){
            prepare_desc( now_pos() , rdstrb.pop() ) ;
            forward_pos() ;
        }
    #endif 
    #ifdef PAGE_FAULT_RESOLVE_TOUCH_ENABLE
        for( int i = batch_base ; i < batch_base + desc_idx ; i ++ ){
            // ignore small transfers, because touch may be random access which causes overhead
            if( descs[i].xfer_size <= 128 * KB ) continue ;
            switch ( descs[i].opcode ) { 
                case DSA_OPCODE_MEMMOVE : // 3
                    touch_trigger_pf( (char*) descs[i].src_addr , 128 * KB , 0 ) ;
                    touch_trigger_pf( (char*) descs[i].dst_addr , 128 * KB , 1 ) ; 
                    break; 
                case DSA_OPCODE_MEMFILL : // 4
                    touch_trigger_pf( (char*) descs[i].dst_addr , 128 * KB , 1 ) ; 
                    break;
                case DSA_OPCODE_COMPARE : // 5
                    touch_trigger_pf( (char*) descs[i].src_addr , 128 * KB , 0 ) ;
                    touch_trigger_pf( (char*) descs[i].src2_addr , 128 * KB , 0 ) ;
                    break;
                case DSA_OPCODE_COMPVAL : // 6
                    touch_trigger_pf( (char*) descs[i].src_addr , 128 * KB , 0 ) ;
                    break;
                default:
                    break;
            } 
        }
    #endif 
    do_op( batch_idx , desc_idx ) ;
    buzy_queue.push_back( batch_idx ) ; 
    batch_idx = free_queue.pop_front() ;
    if( batch_idx == -1 ) {
        batch_base = 0 , desc_idx = 0 ; 
        return ;
    }
    batch_base = batch_idx * batch_siz , desc_idx = 0 ;
    // if( buzy_queue.count() > batch_capacity / 2 ) collect() ;
}

void DSAbatch_task::PF_adjust_desc( int idx ){
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

int DSAbatch_task::resolve_error( int batch_idx ) { 
    int x = op_status( bcomp[batch_idx].status ) ;
    if( x == 1 || x == 0 ) return 0 ; // success or not finish yet
    // printf( "DSA batch op error code 0x%x, %s\n" , x , dsa_comp_status_str( x ) ) ; fflush( stdout ) ;
    batch_fail_cnt ++ ;
    int batch_base = batch_idx * batch_siz ;
    int desc_count = bdesc[batch_idx].desc_count ;
    for( int i = 0 ; i < desc_count ; i ++ ){
        int idx = batch_base + i ;
        uint8_t status = op_status( comps[idx].status ) ;
        // printf( "desc %d, status = 0x%x, %s\n" , idx , status , dsa_comp_status_str( status ) ) ;
        if( status == DSA_COMP_SUCCESS ){ 
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
            comps[idx].status = 0 ;
            // printf( "wr = %d. " , wr ) ;
            #if defined( PAGE_FAULT_RESOLVE_TOUCH_ENABLE )
                char* this_fault = (char*) comps[idx].fault_addr ;
                if( last_fault_addr[idx] && this_fault - last_fault_addr[idx] < DSA_PF_LEN_LIMIT ){ 
                    page_fault_resolving ++ ;
                    int len = descs[idx].xfer_size > DSA_PAGE_FAULT_TOUCH_LEN ? DSA_PAGE_FAULT_TOUCH_LEN : descs[idx].xfer_size ;
                    touch_trigger_pf( this_fault , len , wr ) ;
                }
                last_fault_addr[idx] = this_fault ;
            #endif
        } else if( status == DSA_COMP_NONE ){ // remain the same as before

        } else {
            // not implemented yet
        }
    } 
    // getchar() ;
    return desc_count ;
}

void DSAbatch_task::do_op( int batch_idx , int custom_desc_cnt ) {
    bdesc[batch_idx].desc_count = custom_desc_cnt ; 
    bcomp[batch_idx].status = 0 ;
    submit_desc( wq_portal , ACCFG_WQ_SHARED , &bdesc[batch_idx] ) ; 
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
    alloc_descs() ; 
    return true ;
}

void DSAbatch_task::add_op( dsa_opcode op_type ){ 
    op_cnt ++ ; 
    if( !is_pos_valid() ) {
        printf( "DSAbatch_task::add_op() : full\n" ) ;
        return ;
    }
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        rdstrb.push_back( dsa_rdstrb_entry( op_type ) ) ;
        if( rdstrb.should_submit() ) submit_forward() ;
    #else 
        prepare_desc( now_pos() , dsa_rdstrb_entry( op_type ) ) ; 
        forward_pos() ;
        if( now_batch_full() ) submit_forward() ; 
    #endif 
}

void DSAbatch_task::add_op( dsa_opcode op_type , void *dest , size_t len ){
    op_cnt ++ ;
    op_bytes += len ;
    if( !is_pos_valid() ) {
        printf( "DSAbatch_task::add_op() : full\n" ) ;
        return ;
    } 
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        rdstrb.push_back( dsa_rdstrb_entry( op_type , dest , len ) ) ;
        if( rdstrb.should_submit() ) submit_forward() ;
    #else 
        prepare_desc( now_pos() , dsa_rdstrb_entry( op_type , dest , len ) ) ;   
        forward_pos() ;
        if( now_batch_full() ) submit_forward() ; 
    #endif 
}

void DSAbatch_task::add_op( dsa_opcode op_type , void *dest , const void* src , size_t len ){
    op_cnt ++ ;
    op_bytes += len ;
    if( !is_pos_valid() ) {
        printf( "DSAbatch_task::add_op() : full\n" ) ;
        return ;
    } 
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        rdstrb.push_back( dsa_rdstrb_entry( op_type , dest , src , len ) ) ;
        if( rdstrb.should_submit() ) submit_forward() ;
    #else
        // printf( "len = %d\n" , len ) ;
        prepare_desc( now_pos() , dsa_rdstrb_entry( op_type , dest , src , len ) ) ;
        forward_pos() ;
        if( now_batch_full() ) submit_forward() ; 
    #endif
}

void DSAbatch_task::wait(){
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        if( !rdstrb.empty() ){
            while( rdstrb.can_submit() == false ) add_no_op() ;
            submit_forward() ;
        }
    #else 
        if( desc_idx ){
            while( now_batch_full() == false ) add_no_op() ;
            submit_forward() ; 
        }
    #endif 
    while( buzy_queue.empty() == false ) { collect() ; usleep( 20 ) ; }
}

bool DSAbatch_task::check(){
    #ifdef DESCS_INBATCH_REDISTRIBUTE_ENABLE
        if( !rdstrb.empty() ){
            while( rdstrb.can_submit() == false ) add_no_op() ;
            submit_forward() ;
        }
    #else 
        if( desc_idx ){
            while( now_batch_full() == false ) add_no_op() ;
            submit_forward() ; 
        }
    #endif 
    collect() ;
    return empty() ;
}

bool DSAbatch_task::collect(){ 
    // printf( "collect() : count() = %d( %d + %d )\n" , count() , buzy_queue.count() * batch_siz , desc_idx ) ;
    // buzy_queue.print() ;
    // free_queue.print() ; 
    int recycle_pass = 1 , watch_limit = batch_capacity ;
    #if defined( DESCS_QUEUE_RECYCLE_WINDOW_ENABLE )
        recycle_pass = 1 ;
        watch_limit = QUEUE_RECYCLE_UNFINISHED_LIMIT ;
    #endif

    for( int i = 0 ; i < recycle_pass ; i ++ ) {
        bool recheck = false ;
        for( int q_idx = buzy_queue.q_front , watch_cnt = 0 ;                       // init idx
                q_idx != buzy_queue.q_back && watch_cnt < watch_limit ;             // at most check watch_limit unfinished descs
                q_idx = buzy_queue.next( q_idx ) , watch_cnt ++ ){                  // iterator all element
            int batch_idx = buzy_queue.queue[q_idx] ;
            volatile uint8_t &status = bcomp[batch_idx].status ; 
            if( op_status( status ) > 1 ){ // some error occurred
                buzy_queue.delay_front_and_pop( q_idx ) ;
                int desc_count = resolve_error( batch_idx ) ;
                if( desc_count ) do_op( batch_idx , desc_count ) ;
                buzy_queue.push_back( batch_idx ) ;
            }
            if( op_status( status ) == 1 ){ // success  
                free_queue.push_back( batch_idx ) ; 
                buzy_queue.delay_front_and_pop( q_idx ) ;
                watch_limit ++ ;
            } else if( op_status( status ) == 0 ){ // not finish yet 
                // bool eee = false ; 
                // printf( "wtf ? op_status = %d : " , op_status( bcomp[batch_idx].status ) ) ;
                // for( int i = batch_idx * batch_siz , lim = i + batch_siz ; i < lim ; i ++ ){ 
                //     if( op_status( comps[i].status ) == DSA_COMP_SUCCESS ) { printf_RGB( 0x00ff00 , "S" ) ; eee = true ; }
                //     else { printf( "W" ) ; }
                // }
                // if( eee && op_status( bcomp[batch_idx].status ) == 0 ){
                //     printf( " : wtf ? op_status = %d\n" , op_status( bcomp[batch_idx].status ) );   
                //     getchar() ;
                // } else puts( "" ) ;
                #if defined( DESCS_QUEUE_RECYCLE_WINDOW_ENABLE ) 
                    if( op_status( bcomp[batch_idx].status ) == 1 ){
                        free_queue.push_back( batch_idx ) ;
                        buzy_queue.delay_front_and_pop( q_idx ) ;
                        recheck = true ; 
                    }
                    continue ;
                #else 
                    break ;
                #endif 
            } 
        }
        if( !recheck ) break ;
        // printf( "recheck: " ) ;fflush( stdout ) ;
    }
    if( !is_pos_valid() && !full() ){
        batch_idx = free_queue.pop_front() ;
        batch_base = batch_idx * batch_siz , desc_idx = 0 ;
    } 
    // printf( "after collect() : batch_idx = %d , desc_idx = %d\n" , batch_idx , desc_idx ) ;
    // buzy_queue.print() ;
    // free_queue.print() ; 
    // puts( "\n\n" ) ;
    return is_pos_valid() ;
}
