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
size_t array_len = 1 * GB ;
char *dest_arr = nullptr ;

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

vector<OffLen> genTestset( size_t first_size , size_t array_len ){
    vector<OffLen> rt , following ;
    size_t offset = 0 ; 
    rt.push_back( OffLen( offset , offset , first_size ) ) ;
    offset += first_size ;  
    size_t size_lim = min( 8 * KB , first_size ) ;
    while( true ){
        size_t follow_size = ( rand() % ( size_lim ) ) + 1 ;
        if( offset + follow_size > array_len ) break ;
        rt.push_back( OffLen( offset - follow_size , offset , follow_size ) ) ;
        offset += follow_size ;
    }
    return rt ;
}

uint64_t pattern_ = 0x0f0f0f0f0f0f0f0f ;
char char_patt = pattern_ & 0xff ;
void test_out_of_order( vector<OffLen> testset ){ 
    printf( "test_set.size() = %d\n" , testset.size() ) ;
    dest_arr = (char*) aligned_alloc( 4096 , array_len ) ;  
    for( size_t i = 0 ; i < array_len ; i ++ ) dest_arr[i] = 0 ;
    for( size_t i = testset[0].off_dest ; i < testset[0].off_dest + testset[0].len ; i ++ ) dest_arr[i] = 2 ;
  
    DSA_pure_batch_task *batch_tasks[2000] ; 
    for( int i = 0 ; i < 2000 ; i ++ ) batch_tasks[i] = new DSA_pure_batch_task( DEFAULT_BATCH_SIZE , DSAagent::get_instance().get_wq() ) ; 

    int task_idx = 0 , start_id = 0 ;
    int lim = std::min( (int)testset.size() , 1000 ) ;
    for( int i = 1 ; i < lim ; i ++ ){
        batch_tasks[task_idx]->add_memcpy( dest_arr + testset[i].off_dest , dest_arr + testset[i].off_src , testset[i].len ) ;
        if( batch_tasks[task_idx]->full() ) task_idx ++ ;
    }

    for( start_id = 0 ; start_id <= task_idx ; start_id ++ ){
        batch_tasks[start_id]->do_op() ;  
    }

    for( int i = 0 ; i <= task_idx ; i ++ ){
        while( !batch_tasks[i]->check( true ) ) ;
    }

    for( int i = 1 ; i < lim ; i ++ ){ 
        char* dest = dest_arr + testset[i].off_dest ; 
        bool header = 0 ;
        for( size_t j = 0 ; j < testset[i].len ; j ++ ){ 
            if( dest[j] != 0 ){ 
                if( !header ) {
                    printf( "\ni = %d\n" , i ) ; header = 1 ; 
                    printf( "off_src = %s , off_dest = %s , len = %s\n" , 
                        stdsiz( testset[i].off_src ).c_str() , stdsiz( testset[i].off_dest ).c_str() , stdsiz( testset[i].len ).c_str() ) ;
                }
                printf( "0" ) ; 
            } else if( header ){
                printf( "1" ) ;

            }
        } 
        // if( batch_tasks[0]->check( true ) == true ){ puts("invalid" ) ; break; }
        if( header ) puts( "" ) ;
    }
          
    free( dest_arr ) ;
    for( int i = 0 ; i < 2000 ; i ++ ) delete batch_tasks[i] ; 
} 
 
DSAop ___ ;
int main(){ 
    vector<OffLen> testset = genTestset( 8777 * KB , array_len ) ;
    test_out_of_order( testset ) ; 
    return 0 ;
}