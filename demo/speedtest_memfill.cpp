#include <boost/coroutine2/coroutine.hpp> 
#include "src/async_dsa.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <cmath>
using namespace std ;
typedef boost::coroutines2::coroutine<void> coro_t ;
 
double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;

class dsa_batch_worker{
private:
    std::function<void(void)> yield_out ;
    void *dest ;
    uint64_t pattern ;
    size_t len ;
    int bsiz , tdesc , arr_len ;
    // DSAbatch batch_ ;
    DSAbatch batch_ ;

public: 
    dsa_batch_worker( std::function<void(void)> yield_func_ , 
                        void* dest_ , uint64_t pattern_ , size_t len_ , int bsiz_ , int tdesc_ , int arr_len_ ) : 
    yield_out( yield_func_ ), dest(dest_) , pattern(pattern_) , len(len_) , 
    bsiz( bsiz_ ) , tdesc( tdesc_ ) , arr_len( arr_len_ ) , batch_( bsiz_ , DEFAULT_BATCH_CAPACITY ) { }

    void run() noexcept(true) { 
        yield_out() ;
        for( int i = 0 ; i < tdesc ; i ++ ){
            batch_.submit_memfill( (void*)((uintptr_t) dest + ( 1LL * i * len ) % arr_len ) , pattern , len ) ;
        }  
        yield_out() ; 
        batch_.wait() ; 
        return ;
    }
} ;

string stdsiz( size_t siz ) {
    double regsiz = siz ;
    int regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    char rt[20] ;
    regsiz = floor( regsiz * 1000 + 0.5 ) / 1000 ;
    sprintf( rt , "%4.4g%2s" , regsiz , regstr[regcnt] ) ;
    return string( rt ) ;
}

constexpr int REPEAT = 20 ;
constexpr int bsiz = 64 , tdesc = 4096 , MEMCPY_CNT = bsiz ;
constexpr size_t ARRAY_LEN = 1073741824 ;
char c[ARRAY_LEN]  ; 
int main(){   
    char *a = (char*)aligned_alloc( 2 , ARRAY_LEN ) ; 
    printf( "a @ %p \n" , a ) ;
    // if( ( (uintptr_t)a & 7 ) == 0 ) a += 5 ; 
    for( size_t i = 0 ; i < ARRAY_LEN ; i ++ ) a[i] = i ; 
    for( size_t i = 0 ; i < ARRAY_LEN ; i += 4096 ) c[i] = 2 ;

    for( size_t len = 64 , warmup = 0 ; len <= 4 * MB ; len *= 2 ){ 
        double st_time , ed_time , alloc_time , submit_time , do_time ;  
         
        // for( int i = 0 ; i < ARRAY_LEN ; i ++ ) c[i] = b[i] + c[ARRAY_LEN-1-i] ; // flush cache
        double memcpy_time = 0 , memcpy_speed = 0 ;
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){
            st_time = timeStamp_hires() ;  
            for( int i = 0 ; i < tdesc ; i ++ ) memset( a + ( 1LL * i * len ) % ARRAY_LEN , 0 , len ) ; 
            ed_time = timeStamp_hires() , memcpy_time += ( ed_time - st_time ) / REPEAT / ( tdesc ) ;
        }
        memcpy_time *= ns_to_us , memcpy_speed = len / ( memcpy_time * us_to_s ) / MB ;

        // batch op
        double DSA_batch_alloc = 0 , DSA_batch_do = 0 , DSA_batch_submit = 0 ;
        double DSA_batch_time = 0 , DSA_batch_speed = 0 ;
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
            st_time = timeStamp_hires() ;
            coro_t::pull_type batch_yield(
                [&]( coro_t::push_type &yield_func ){
                    dsa_batch_worker worker( [&yield_func](){ yield_func() ; } ,
                    a , 0 , len , bsiz , tdesc , ARRAY_LEN ) ;
                    worker.run() ;
                }
            ) ; 
            ed_time = timeStamp_hires() , alloc_time = ed_time - st_time , st_time = ed_time ; 
            batch_yield() ; 
            ed_time = timeStamp_hires() , submit_time = ed_time - st_time , st_time = ed_time ;
            batch_yield() ;
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ;
            DSA_batch_alloc += ( alloc_time ) / REPEAT / tdesc ; 
            DSA_batch_submit += ( submit_time ) / REPEAT / tdesc ;
            DSA_batch_do += ( do_time ) / REPEAT / tdesc ;
        }
        DSA_batch_alloc *= ns_to_us , DSA_batch_do *= ns_to_us , DSA_batch_submit *= ns_to_us ;
        DSA_batch_time = DSA_batch_alloc + DSA_batch_do + DSA_batch_submit ;
        DSA_batch_speed = len / ( DSA_batch_time * us_to_s ) / MB ; 

        double pure_batch_do = 0 , pure_batch_submit = 0 ;
        double pure_batch_time = 0 , pure_batch_speed = 0 ;
        DSAbatch xfer( bsiz , DEFAULT_BATCH_CAPACITY ) ; 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
            xfer.db_task.clear() ;
            st_time = timeStamp_hires() ;
            for( int i = 0 ; i < tdesc ; i ++ ) xfer.submit_memfill( a + ( 1LL * i * len ) % ARRAY_LEN , 0 , len ) ;
            // dsa_completion_record *ccc = xfer.db_task.get_front_comp() ;
            ed_time = timeStamp_hires() , submit_time = ed_time - st_time , st_time = ed_time ;
            // while( ccc->status == 0 ) ;
            xfer.db_task.wait() ;
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            pure_batch_submit += ( submit_time ) / REPEAT / tdesc ;
            pure_batch_do += ( do_time ) / REPEAT / tdesc ;
        }
        pure_batch_do *= ns_to_us , pure_batch_submit *= ns_to_us ;
        pure_batch_time = pure_batch_do + pure_batch_submit ;
        pure_batch_speed = len / ( pure_batch_time * us_to_s ) / MB ;
        if( !warmup ) { ++warmup ; len /= 2 ; continue ;}
 
        printf( "Fill %s * %d desc, tot %s\n" , stdsiz( len ).c_str() , tdesc , stdsiz( len * tdesc ).c_str() ) ;
        printf( "    ; memset     cost %5.2lfus, speed %5.0fMB/s\n" 
                "    ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n"
                "    ;            |- allc %7.2lfus\n"  
                "    ;            |- subm %7.2lfus\n" 
                "    ;            |- wait %7.2lfus\n" 
                "    ; pure_batch cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n" 
                "    ;            |- subm %7.2lfus\n" 
                "    ;            |- wait %7.2lfus\n"
                "\n", 
                memcpy_time , memcpy_speed , 
                DSA_batch_time , DSA_batch_speed , memcpy_time - DSA_batch_time ,
                DSA_batch_alloc , 
                DSA_batch_submit ,
                DSA_batch_do , 
                pure_batch_time , pure_batch_speed , memcpy_time - pure_batch_time ,
                pure_batch_submit , 
                pure_batch_do ) ; 
        fflush( stdout ) ;
    } 
}

// g++ coroutine2_dsa.cpp -o coroutine2_dsa -lboost_coroutine -lboost_context -laccel-config -Wall -mcmodel=large