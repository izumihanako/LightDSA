#include "src/async_dsa.hpp" 
#include "src/details/dsa_pure_batch_task.hpp"

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
constexpr int REPEAT = 1000 ;  
int method = 0 ;
size_t array_len = 1 * GB , follow_size = 1024 ;
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

vector<OffLen> genTestset( size_t first_size , size_t follow_size , size_t array_len ){
    vector<OffLen> rt , following ;
    size_t offset = 0 , follow_begin = 0 ; 
    rt.push_back( OffLen( offset , offset , first_size ) ) ;
    offset += first_size ; 
    follow_begin = offset ;
    for( int i = 1 ; i < 40000 ; i ++ ){
        if( offset + follow_size > array_len ) offset = follow_begin ;
        rt.push_back( OffLen( offset , offset , follow_size ) ) ;
        offset += follow_size ;
    }
    return rt ;
}

uint64_t pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_ & 0xff ;
void test_out_of_order( int method , vector<OffLen> testset , int op_type ){
    src_arr = (char*) aligned_alloc( 4096 , array_len ) ;
    dest_arr = (char*) aligned_alloc( 4096 , array_len ) ; 
    if( op_type == 0 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = i , dest_arr[i] = 0 ; 
    } else if( op_type == 1 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = 0 ;
    }else if( op_type == 2 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = dest_arr[i] = i ;
    } else if( op_type == 3 ){
        for( size_t i = 0 ; i < array_len ; i ++ ) src_arr[i] = char_patt ;
    }

    double ooo_comp_avg_idx = 0 ; 
    char *states = new char[40000] ;  
    DSA_pure_batch_task *batch_tasks[2000] ;
    DSAtask *tasks[40000] ;
    if( method == 1 ) for( int i = 0 ; i < 2000 ; i ++ ) batch_tasks[i] = new DSA_pure_batch_task( DEFAULT_BATCH_SIZE , DSAagent::get_instance().get_wq() ) ;
    else for( int i = 0 ; i < 40000 ; i ++ ) tasks[i] = new DSAtask( DSAagent::get_instance().get_wq() ) ;

    for( int repeat = 0 , warmup = 10 ; repeat < REPEAT ; repeat ++ ){
        memset( states , 0 , 40000 ) ; 
        // setting works
        int task_idx = 0 , cnt_finished = 0 , start_id = 0 ;
        if( method == 1 ){
            for( size_t i = 0 ; i < testset.size() ; i ++ ){
                if( op_type == 0 ) batch_tasks[task_idx]->add_memcpy( dest_arr + testset[i].off_dest , src_arr + testset[i].off_src , testset[i].len ) ;
                else if( op_type == 1 ) batch_tasks[task_idx]->add_memfill( dest_arr + testset[i].off_dest , pattern_ , testset[i].len ) ;
                else if( op_type == 2 ) batch_tasks[task_idx]->add_compare( dest_arr + testset[i].off_dest , src_arr + testset[i].off_src , testset[i].len ) ;
                else if( op_type == 3 ) batch_tasks[task_idx]->add_comp_pattern( src_arr + testset[i].off_src , pattern_ , testset[i].len ) ;
                if( batch_tasks[task_idx]->full() ) task_idx ++ ;
            } 
            int check_start_point = 1;
            for( start_id = 0 ; start_id <= task_idx ; start_id ++ ){
                batch_tasks[start_id]->check( true ) ;
                if( batch_tasks[0]->check( true ) == true ) break ;
                if( start_id & 3 ) continue ; // check every 4 submit
                int sequence_cnt = 0 ;
                for( int i = check_start_point ; i <= start_id && batch_tasks[0]->get_status() == 0 ; i ++ ){
                    if( batch_tasks[i]->get_status() == 1 ){
                        sequence_cnt ++ ;
                        if( sequence_cnt > 30 ) check_start_point = i ;
                        states[i] = 1 ;
                    } else {
                        sequence_cnt = 0 ;
                        states[i] = 0 ;
                    }
                } 
            }
            for( int i = 0 ; i <= start_id ; i ++ ) cnt_finished += states[i] ; 
            for( int i = 0 ; i <= task_idx ; i ++ ) {
                if( batch_tasks[i]->is_doing() ) batch_tasks[i]->wait() ;
                else batch_tasks[i]->clear() ; 
            }
        } else if( method == 0 ){  
            for( size_t i = 0 ; i < testset.size() ; i ++ ){
                if( op_type == 0 ) tasks[task_idx]->set_op( DSA_OPCODE_MEMMOVE , dest_arr + testset[i].off_dest , src_arr + testset[i].off_src , testset[i].len ) ;
                else if( op_type == 1 ) tasks[task_idx]->set_op( DSA_OPCODE_MEMFILL , dest_arr + testset[i].off_dest , (void*) pattern_ , testset[i].len ) ;
                else if( op_type == 2 ) tasks[task_idx]->set_op( DSA_OPCODE_COMPARE , dest_arr + testset[i].off_dest , src_arr + testset[i].off_src , testset[i].len ) ;
                else if( op_type == 3 ) tasks[task_idx]->set_op( DSA_OPCODE_COMPVAL , (void*) pattern_ , src_arr + testset[i].off_src , testset[i].len ) ;
                task_idx ++ ;
            } task_idx -- ;
            int check_start_point = 1; 
            for( start_id = 0 ; start_id <= task_idx ; start_id ++ ){
                tasks[start_id]->do_op() ; 
                if( tasks[0]->check() == true ) break ; 
                int sequence_cnt = 0 ;
                for( int i = check_start_point ; i <= start_id && tasks[0]->get_comp_status() == 0 ; i ++ ){
                    if( tasks[i]->get_comp_status() == 1 ){
                        sequence_cnt ++ ;
                        if( sequence_cnt > 30 ) check_start_point = i ;
                        states[i] = 1 ;
                    } else {
                        sequence_cnt = 0 ;
                        states[i] = 0 ;
                    }
                } 
            }
            for( int i = 0 ; i <= start_id ; i ++ ) cnt_finished += states[i] ;
            for( int i = 0 ; i <= task_idx ; i ++ ) {
                while( !tasks[i]->check() ) ;
            }
        }
        // for( int i = 0 ; i <= start_id ; i ++ ){ 
        //     if( states[i] == DSA_COMP_SUCCESS ) printf_RGB( 0x00ff00 , "S" ) ;
        //     else printf( "W" ) ;
        // } puts( "" ) ;
        // printf( "last finished: %d\n" , cnt_finished ) ; 
        if( warmup <= 10 ){ warmup ++ ; repeat -- ; continue ;}
        ooo_comp_avg_idx += 1.0 * cnt_finished / REPEAT ; 
    } 
    printf( "Average out-of-order completed batch: %.1f\n" , ooo_comp_avg_idx ) ; fflush( stdout ) ;
    free( src_arr ) ;
    free( dest_arr ) ;
    if( method == 1 ) for( int i = 0 ; i < 1000 ; i ++ ) delete batch_tasks[i] ;
    else for( int i = 0 ; i < 30000 ; i ++ ) delete tasks[i] ; 
} 
 
DSAop ___ ;
int main(){ 
    for( int op_type = 0 ; op_type <= 3 ; op_type ++ ){
        for( size_t first_size = 64 * KB ; first_size <= 8 * MB ; first_size *= 2 ){ 
            printf( "DSA_batch , op_type = %s , first_size: %s, follow_size: %s, REPEAT = %d\n" , 
                op_type == 0 ? "MEMMOVE" : op_type == 1 ? "MEMFILL" : op_type == 2 ? "COMPARE" : "COMPVAL" ,
                stdsiz( first_size ).c_str() , stdsiz( follow_size ).c_str() , REPEAT ) ;
            vector<OffLen> testset = genTestset( first_size , follow_size , array_len) ;
            test_out_of_order( method , testset , op_type ) ;
        }
        puts( "" ) ;
    }
    return 0 ;
}