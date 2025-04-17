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
    bsiz = 0 ;
    nega_cnt = posi_cnt = 0 ;
    credit = 0 ;
    counter = 0 ;
}

DSAtask_redistribute::DSAtask_redistribute( int bsiz_ ){
    init( bsiz_ ) ;
}

DSAtask_redistribute::~DSAtask_redistribute(){
    if( nega_entries ) free( nega_entries ) ;
    if( posi_entries ) free( posi_entries ) ;
    nega_entries = posi_entries = nullptr ;
}

void DSAtask_redistribute::init( int bsiz_ ){ 
    bsiz = bsiz_ ;
    nega_entries = (dsa_rdstrb_entry*) malloc( sizeof(dsa_rdstrb_entry) * bsiz ) ;
    posi_entries = (dsa_rdstrb_entry*) malloc( sizeof(dsa_rdstrb_entry) * bsiz ) ; 
    if( nega_entries == nullptr || posi_entries == nullptr ){
        printf( "DSAtask_redistribute::DSAtask_redistribute() : memory allocation failed\n" ) ;
        exit( 1 ) ;
    }
    nega_cnt = posi_cnt = 0 ;
    credit = 0 ;
    counter = 0 ;
}

constexpr uint64_t REDISTRIBUTE_CREDIT_THRESHOLD = 8 * KB ;  
void DSAtask_redistribute::push_back( const dsa_rdstrb_entry &entry ){
    if( entry.xfer_size < REDISTRIBUTE_CREDIT_THRESHOLD ) {
        nega_entries[nega_cnt++] = entry ;
    } else {
        posi_entries[posi_cnt++] = entry ;
    }
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
    0, 0, 0, 0, 0, 0, 0, 0, 0,  // < 512
    -2, -2, -2,                 // 512 ~ 4KB-1
    -1,                         // 4KB ~ 8KB
    1,                          // 8KB ~ 16KB-1
    4,                          // 16KB ~ 32KB-1
    8, 8, 8,                    // 32KB ~ 256KB-1
    16,                         // 256KB ~ 512KB-1
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30
}; 
dsa_rdstrb_entry DSAtask_redistribute::pop(){
    if( counter == 0 ){ // new batch
        counter = bsiz ;
        credit = 0 ; 
        // getchar() ;
    }
    counter -- ;
    int highest_bit = 0 ;
    int credit_delta = 0 ;
    dsa_rdstrb_entry *rtpt = nullptr ;
    if( credit > 0 && nega_cnt ){ 
        if( nega_entries[nega_cnt-1].xfer_size >= 4 * KB ) credit_delta = -1 ;
        else credit_delta = -2 ;  
        rtpt = &nega_entries[--nega_cnt] ;
    } else {
        if( posi_cnt ){
            highest_bit =  31 - __builtin_clz( posi_entries[posi_cnt-1].xfer_size ) ;
            credit_delta = credit_table[highest_bit] ;
            rtpt = &posi_entries[--posi_cnt] ;
        }
        else if( nega_cnt ) rtpt = &nega_entries[--nega_cnt] , credit_delta = 0 ;
    }
    // printf( "pop() : xfer_len = %d, nega_cnt = %d, posi_cnt = %d\n" , rtpt->xfer_size , nega_cnt , posi_cnt ) ;
    credit += credit_delta ;
    // printf( "%d" , credit ) ;
    // if( credit_delta <= 0 ) printf_RGB( 0x775500 + credit * 0x33dd00 , "-") ;
    // else                    printf_RGB( 0x775500 + ( 31 - __builtin_clz( credit ) ) * 0xff2200 , "+" ) ;
    // fflush( stdout ) ;
    if( rtpt ) return *rtpt ;
    
    puts( "DSAtask_redistribute::pop() : no more entries" ) ;
    return dsa_rdstrb_entry( DSA_OPCODE_NOOP ) ;
}