#include "src/async_dsa.hpp" 

#include <cstdio>
#include <cstring>
#include <cstdlib> 
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
using namespace std ;

double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;
constexpr int REPEAT = 10 ; 
int method = 0 , desc_cnt = 0 , op_type = 0 ;
size_t array_len = 4 * GB ;
char* src_arr = nullptr , *dest_arr = nullptr ;

struct OffLen{
    size_t off_src ;
    size_t off_dest ;
    size_t len ;
    OffLen( size_t off_src_ , size_t off_dest_ , size_t len_ ) : off_src( off_src_ ) , off_dest( off_dest_ ) , len( len_ ) {} 
} ;

string stdsiz( size_t siz ) {
    double regsiz = siz ;
    int regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    char rt[20] ;
    regsiz = floor( regsiz * 1000 + 0.5 ) / 1000 ;
    sprintf( rt , "%.4g%2s" , regsiz , regstr[regcnt] ) ;
    return string( rt ) ;
}

vector<OffLen> genTestset( int desc_cnt , size_t array_len , size_t transfer_size ){
    vector<OffLen> rt ;
    size_t offset = 0 ;
    for( int i = 0 ; i < desc_cnt ; i ++ ){
        if( offset + transfer_size > array_len ) offset = 0 ;
        rt.push_back( OffLen( offset , offset , transfer_size ) );
        offset += transfer_size ;
    }
    // random_shuffle( rt.begin() , rt.end() ) ;
    return rt ;
}

uint64_t pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_ & 0xff ;
void test_dsa_speed( int alignment = 64 ){ 
    char* _src = (char*) aligned_alloc( 4096 , array_len + MB ) ;
    char* _dest = (char*) aligned_alloc( 4096 , array_len + MB ) ;
    src_arr = _src , dest_arr = _dest ;
    for( size_t delta = 1 , t = 1 ; delta < 4096 ; delta *= 2 , t ++ ){
        if( ( t & 1 ) && delta > (size_t)alignment ){
            src_arr += delta ;
            dest_arr += delta ;
        }
        if( delta == (size_t)alignment ) {
            src_arr += alignment ;
            dest_arr += alignment ;
        }
    } 


    printf( "method = %s, op = %s, desc_cnt = %d, array_len = %s, alignment = %d bytes\n"
            "src_arr @ %p , dest_arr @ %p\n" , 
            method == 0 ? "DSA_batch" : method == 1 ? "DSA_memcpy*32" : "DSA_single" ,
            op_type == 0 ? "memcpy" : op_type == 1 ? "memfill" : op_type == 2 ? "compare" : "compval" ,
            desc_cnt , stdsiz( array_len ).c_str() , alignment , src_arr , dest_arr ) ;

    DSAbatch dsa_batch( 32 , 20 ) ;
    DSAop    dsa_op , dsa_ops[128] ;
    for( size_t transfer_size = 64 ; transfer_size <= 1 * MB ; transfer_size *= 2 ){
        if( op_type == 0 ){
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i ; 
        } else if( op_type == 1 ){
            for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = 0 ;
        }else if( op_type == 2 ){
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = i ;
        } else if( op_type == 3 ){
            for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ;
        } 
        vector<OffLen> test_set = genTestset( desc_cnt , array_len , transfer_size ) ;
        size_t tot_xfersize = 0 ;
        for( auto& it : test_set ) tot_xfersize += it.len ;

        double do_time = 0 , do_speed = 0 ;
        for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){
            for( size_t i = 0 ; i < array_len ; i += 4096 ) {
                src_arr[i] = src_arr[i] ^ 0;
                dest_arr[i] = dest_arr[i] ^ 0 ;
            }
            uint64_t start = timeStamp_hires() ;
            if( method == 0 ) { // DSA_batch
                for( auto& it : test_set ){
                    if( op_type == 0 ) dsa_batch.submit_memmove( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                    else if( op_type == 1 ) dsa_batch.submit_memfill( dest_arr + it.off_dest , pattern_ , it.len ) ;
                    else if( op_type == 2 ) dsa_batch.submit_compare( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                    else if( op_type == 3 ) dsa_batch.submit_comp_pattern( src_arr + it.off_src , pattern_ , it.len ) ;
                }
                dsa_batch.wait() ;
            } else if( method == 1 ){ // memcpy * 128
                for( int id = 0 , lim = test_set.size() ; id < lim ; id ++ ){
                    int dsa_id = id % 128 ;
                    dsa_ops[dsa_id].wait() ;
                    if( op_type == 0 ) dsa_ops[dsa_id].async_memmove( dest_arr + test_set[id].off_dest , src_arr + test_set[id].off_src , test_set[id].len ) ;
                    else if( op_type == 1 ) dsa_ops[dsa_id].async_memfill( dest_arr + test_set[id].off_dest , pattern_ , test_set[id].len ) ;
                    else if( op_type == 2 ) dsa_ops[dsa_id].async_compare( dest_arr + test_set[id].off_dest , src_arr + test_set[id].off_src , test_set[id].len ) ;
                    else if( op_type == 3 ) dsa_ops[dsa_id].async_comp_pattern( src_arr + test_set[id].off_src , pattern_ , test_set[id].len ) ;
                }
                for( int dsa_id = 0 ; dsa_id < 128 ; dsa_id ++ ) dsa_ops[dsa_id].wait() ;  
            } else if( method == 2 ){
                for( auto& it : test_set ){
                    if( op_type == 0 ) dsa_op.sync_memcpy( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                    else if( op_type == 1 ) dsa_op.sync_memfill( dest_arr + it.off_dest , pattern_ , it.len ) ;
                    else if( op_type == 2 ) dsa_op.sync_compare( dest_arr + it.off_dest , src_arr + it.off_src , it.len ) ;
                    else if( op_type == 3 ) dsa_op.sync_comp_pattern( src_arr + it.off_src , pattern_ , it.len ) ;
                }  
            }
            if( warmup <= 2 ){
                warmup ++ ; repeat -- ; continue ;
            }
            uint64_t end = timeStamp_hires() ;
            do_time += ( end - start ) / REPEAT ;
        }
        if( op_type == 0 ) {
            for( int i = 0 , lim = min( tot_xfersize , array_len ) ; i < lim ; i ++ )
                if( src_arr[i] != dest_arr[i] ) {
                    printf( "Error: src[%d] = %d , dest[%d] = %d\n" , i , src_arr[i] , i , dest_arr[i] ) ;
                    break ;
                }
        } else if( op_type == 1 ){
            for( int i = 0 , lim = min( tot_xfersize , array_len ) ; i < lim ; i ++ )
                if( char_patt != dest_arr[i] ) {
                    printf( "Error: src[%d] = %d , dest[%d] = %d\n" , i , src_arr[i] , i , dest_arr[i] ) ;
                    break ;
                }
        } else if( op_type == 2 || op_type == 3 ){
            for( int i = 0 ; i < 32 ; i ++ )
                if( dsa_ops[i].compare_res() ) {
                    printf( "Compare result error, differ pos is %lu" , dsa_ops[i].compare_differ_offset() ) ;
                }
            if( dsa_op.compare_res() ){
                printf( "Compare result error, differ pos is %lu" , dsa_op.compare_differ_offset() ) ;
            }
        }
        do_time *= ns_to_us ; // us
        do_time /= desc_cnt ; // single desc time
        do_speed = transfer_size / ( do_time * us_to_s ) / MB ; // GB/s
        printf( "transfer_size = %6s | do_time = %5.2f us | do_speed = %5.0f MB/s | REPEAT = %d\n" , 
                stdsiz( transfer_size ).c_str() , do_time , do_speed , REPEAT ) ; fflush( stdout ) ;
        // if( method == 0 ) dsa_batch.print_stats() ;
        // else if( method == 1 ) dsa_ops[0].print_stats() ;
        // else if( method == 2 ) dsa_op.print_stats() ;
    }
    free( _src ) ;
    free( _dest ) ;
} 
 
DSAop ___ ;
int main( int argc , char** argv ){ 
    if( argc < 4 ){ 
        printf("Usage: %s <method> <op> <desc_cnt>\n", argv[0]) ;
        printf( "method    : 0 is DSA_batch, 1 is DSA_memcpy*32, 2 is DSA_single\n" ) ;
        printf( "op        : 0 is memcpy, 1 is memfill, 2 is compare, 3 is compval\n" ) ;
        printf( "desc_cnt  : number of descs\n" ) ; 
        return 0 ;
    } else {
        method = atoi( argv[1] ) ; 
        op_type = atoi( argv[2] ) ;
        desc_cnt = atoi( argv[3] ) ; 
    } 
    for( int alignment = 8 ; alignment <= 128 ; alignment *= 2 ){ 
        test_dsa_speed( alignment) ;
        puts( "" ) ;
    } 
    return 0 ;
}