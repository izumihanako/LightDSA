#include "dsa_batch_redistribute.hpp"
#include "dsa_util.hpp"
#include "util.hpp"

#include <cstdio> 
#include <cstdlib>

dsa_rdstrb_entry::dsa_rdstrb_entry( dsa_opcode op ){
    opcode = op ;
    xfer_size = 0 ;  
    switch ( op ) {
    case DSA_OPCODE_NOOP :
        src_addr = dst_addr = 0 ;
        flags = DSA_NOOP_FLAG ;
        break ;   
    default :
        printf( "dsa_rdstrb_entry: op(%s) Unimplemented\n" , dsa_op_str(op) ) ;
        exit( 1 ) ;
    }
}

dsa_rdstrb_entry::dsa_rdstrb_entry( dsa_opcode op , void* src , uint64_t len , uint64_t stride ){
    opcode = op ;
    xfer_size = len ;  
    switch ( op ) {
    case DSA_OPCODE_TRANSL_FETCH:
        region_stride = stride ;
        src_addr = (uint64_t)src ;
        flags = DSA_TRANSL_FETCH_FLAG ;
        if( stride ) flags |= ( 1 << 16 ) ;
        break; 
    case DSA_OPCODE_CFLUSH :
        src_addr = 0 ;
        dst_addr = (uint64_t)src ;
        flags = DSA_CFLUSH_FLAG ;
        break; 
    default :
        printf( "dsa_rdstrb_entry: op(%s) Unimplemented\n" , dsa_op_str(op) ) ;
        exit( 1 ) ;
    }
}

dsa_rdstrb_entry::dsa_rdstrb_entry( dsa_opcode op , void* dest , const void* src , uint64_t len ){
    opcode = op ;
    xfer_size = len ;  
    switch ( op ) {
    case DSA_OPCODE_MEMMOVE :
        src_addr = (uint64_t)src ;
        dst_addr = (uint64_t)dest ;
        flags = DSA_MEMMOVE_FLAG ;
        break;
    case DSA_OPCODE_MEMFILL :
        pattern_lower = (uint64_t)src ;
        dst_addr = (uint64_t)dest ;
        flags = DSA_MEMFILL_FLAG ;
        break ;
    case DSA_OPCODE_COMPARE :
        src_addr = (uint64_t)src ;
        src2_addr = (uint64_t)dest ;
        flags = DSA_COMPARE_FLAG ;
        break;
    case DSA_OPCODE_COMPVAL :
        src_addr = (uint64_t)src ;
        comp_pattern = (uint64_t)dest ;
        flags = DSA_COMPVAL_FLAG ;
        break;
    default :
        printf( "dsa_rdstrb_entry: op(%s) Unimplemented\n" , dsa_op_str(op) ) ;
        exit( 1 ) ;
    }
}

DSAtask_redistribute::DSAtask_redistribute(){
    nega_entries = posi_entries = nullptr ;
    nega_credits = posi_credits = nullptr ;
    bsiz = 0 ;
    nega_cnt = posi_cnt = 0 ;
    credit_fix = credit = 0 ;
    sum_credit = 0 ;
    counter = 0 ;
}

DSAtask_redistribute::DSAtask_redistribute( int bsiz_ ){
    init( bsiz_ ) ;
}

DSAtask_redistribute::~DSAtask_redistribute(){
    if( nega_entries ) free( nega_entries ) ;
    if( posi_entries ) free( posi_entries ) ;
    if( posi_credits ) free( posi_credits ) ;
    if( nega_credits ) free( nega_credits ) ;
    nega_entries = posi_entries = nullptr ;
    posi_credits = nega_credits = nullptr ;
}

void DSAtask_redistribute::init( int bsiz_ ){ 
    bsiz = bsiz_ ;
    nega_entries = (dsa_rdstrb_entry*) malloc( sizeof(dsa_rdstrb_entry) * bsiz ) ;
    posi_entries = (dsa_rdstrb_entry*) malloc( sizeof(dsa_rdstrb_entry) * bsiz ) ; 
    posi_credits = (int8_t*) malloc( sizeof(uint8_t) * bsiz ) ;
    nega_credits = (int8_t*) malloc( sizeof(uint8_t) * bsiz ) ;
    if( nega_entries == nullptr || posi_entries == nullptr || 
        nega_credits == nullptr || posi_credits == nullptr ) {
        printf( "DSAtask_redistribute::DSAtask_redistribute() : memory allocation failed\n" ) ;
        exit( 1 ) ;
    }
    nega_cnt = posi_cnt = 0 ;
    credit_fix = credit = 0 ;
    sum_credit = 0 ;
    counter = 0 ;
}

// 512KB credit 30
// 256KB credit 16
//  32KB credit 8
//  16KB credit 4
//   8KB credit 1
//   4KB credit -1
//   2KB credit -2
//  <512 use CPU
static const int8_t credit_table[32] = {
    0, 0, 0, 0, 0, 0, 0,-2,-2,  // < 512
    -2, -2, -2,                 // 512 ~ 4KB-1
    -1,                         // 4KB ~ 8KB
    1,                          // 8KB ~ 16KB-1
    4,                          // 16KB ~ 32KB-1
    8, 8, 8,                    // 32KB ~ 256KB-1
    16,                         // 256KB ~ 512KB-1
    30, 35, 40, 45, 50, 55, 60, 60, 60, 60, 60, 60
}; 
constexpr uint64_t REDISTRIBUTE_CREDIT_THRESHOLD = 8 * KB ;  
void DSAtask_redistribute::push_back( const dsa_rdstrb_entry &entry ){
    if( entry.xfer_size < REDISTRIBUTE_CREDIT_THRESHOLD ) {
        if( entry.xfer_size >= 4 * KB ) nega_credits[nega_cnt] = -1 ;
        else nega_credits[nega_cnt] = -2 ;
        sum_credit += nega_credits[nega_cnt] ; 
        nega_entries[nega_cnt++] = entry ;
    } else {
        int highest_bit =  31 - __builtin_clz( entry.xfer_size ) ;
        posi_credits[posi_cnt] = credit_table[highest_bit] ; 
        sum_credit += posi_credits[posi_cnt] ;
        posi_entries[posi_cnt++] = entry ;
    }
}

dsa_rdstrb_entry DSAtask_redistribute::pop(){
    if( counter == 0 ){ // new batch
        counter = bsiz ;
        credit = 0 ; 
        credit_fix = - 1.0 * sum_credit / bsiz ;
        sum_credit = 0 ;
    }
    counter -- ;
    int credit_delta = 0 ;
    dsa_rdstrb_entry *rtpt = nullptr ;
    if( credit > 0 && nega_cnt ){ 
        credit_delta = nega_credits[--nega_cnt] ; 
        rtpt = &nega_entries[nega_cnt] ;
    } else if( posi_cnt ){
        credit_delta = posi_credits[--posi_cnt] ; 
        rtpt = &posi_entries[posi_cnt] ;
    } else if( nega_cnt ){
        credit_delta = 0 ;
        rtpt = &nega_entries[--nega_cnt] ;
    }
    // printf( "pop() : xfer_len = %d, nega_cnt = %d, posi_cnt = %d\n" , rtpt->xfer_size , nega_cnt , posi_cnt ) ;
    credit += credit_delta + credit_fix ;
    // // printf( "%.2f" , credit ) ;
    // if( credit_delta <= 0 ) printf_RGB( 0x775500 + credit_delta * 0x33dd00 , "-") ;
    // else                    printf_RGB( 0x775500 + ( 31 - __builtin_clz( credit_delta ) ) * 0xff2200 , "+" ) ;
    // fflush( stdout ) ;
    if( rtpt ) return *rtpt ;
    
    puts( "DSAtask_redistribute::pop() : no more entries" ) ;
    return dsa_rdstrb_entry( DSA_OPCODE_NOOP ) ;
}