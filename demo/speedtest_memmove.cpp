#include <boost/coroutine2/coroutine.hpp> 
#include "src/async_dsa.hpp"

#include <numa.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <cmath>
using namespace std ;
typedef boost::coroutines2::coroutine<void> coro_t ;

class yield_calibrator{
private:
    std::function<void(void)> yield_out ; 
    int N ;

public: 
    yield_calibrator( std::function<void(void)> yield_func_ , int n ): yield_out( yield_func_ ) , N( n ) {} ;

    void run() {
        for( int i = 1 ; i <= N ; i ++ ){
            yield_out() ; 
        } 
        return ;
    }
} ;

class dsa_memmove_worker{
private:
    std::function<void(void)> yield_out ;
    void *dest , *src ;
    size_t len ;
    DSAop memcpy_ ;

public: 
    dsa_memmove_worker( std::function<void(void)> yield_func_ , 
                        void* dest_ , void* src_ , size_t len_ ) : 
    yield_out( yield_func_ ), dest(dest_) , src(src_) , len(len_) { }

    void* run() noexcept(true) {
        yield_out() ;
        memcpy_.sync_memcpy( dest , src , len ) ; 
        return dest ; 
    }
} ;

double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;

class dsa_batch_worker{
private:
    std::function<void(void)> yield_out ;
    void *dest , *src ;
    size_t len ;
    int bsiz , tdesc , arr_len ;
    // DSAbatch batch_ ;
    DSAbatch batch_ ;

public: 
    dsa_batch_worker( std::function<void(void)> yield_func_ , 
                        void* dest_ , void* src_ , size_t len_ , int bsiz_ , int tdesc_ , int arr_len_ ) : 
    yield_out( yield_func_ ), dest(dest_) , src(src_) , len(len_) , 
    bsiz( bsiz_ ) , tdesc( tdesc_ ) , arr_len( arr_len_ ) , batch_( bsiz_ , DEFAULT_BATCH_CAPACITY ) { }

    void run() noexcept(true) { 
        yield_out() ;
        for( int i = 0 ; i < tdesc ; i ++ ){
            batch_.submit_memcpy( (void*)((uintptr_t) dest + ( 1LL * i * len ) % arr_len ) , (void*)((uintptr_t)src + ( 1LL * i * len ) % arr_len ) , len ) ;
        }  
        yield_out() ; 
        batch_.wait() ; 
        return ;
    }
} ;

double create_co_overhead = 0 ;
double yield_overhead = 0 ;
void calibrate_yield(){ 
    static int N = 1000000 ;
    double st , ed ;
    for( int i = 0 ; i < N ; i ++ ){
        st = timeStamp_hires() ;
        boost::coroutines2::coroutine<void>::pull_type yield(
            [&]( boost::coroutines2::coroutine<void>::push_type &yield_func ){
                yield_calibrator worker( [&yield_func](){ yield_func() ; } , 1 ) ;
                worker.run() ;
            }
        ) ;
        yield() ;
        ed = timeStamp_hires() ; 
        create_co_overhead += ( ed - st ) * ns_to_us / N ;
    }
    printf( "create time : %.2fus\n" , create_co_overhead ) ; 

    st = timeStamp_hires() ;
    boost::coroutines2::coroutine<void>::pull_type yield(
        [&]( boost::coroutines2::coroutine<void>::push_type &yield_func ){
            yield_calibrator worker( [&yield_func](){ yield_func() ; } , N ) ;
            worker.run() ;
        }
    ) ;    
    for( int i = 0 ; i < N ; i ++ ){
        yield() ;
    }
    ed = timeStamp_hires() ;
    yield_overhead =  ( ed - st ) * ns_to_us / N ;
    printf( "yield time : %.2fus\n" , yield_overhead ) ; 
}

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
constexpr int bsiz = DEFAULT_BATCH_SIZE , tdesc = 4096 , MEMCPY_CNT = bsiz ;
constexpr size_t ARRAY_LEN = 1 * GB ;
char c[ARRAY_LEN]  ; 
int main(){  
    calibrate_yield() ;
    // char *a_ = (char*)numa_alloc_onnode( ARRAY_LEN , 0 ) , *a = (char*)( (uintptr_t)( a_ + 0x3f ) & ~0x3f ) ;
    // char *b_ = (char*)numa_alloc_onnode( ARRAY_LEN , 1 ) , *b = (char*)( (uintptr_t)( b_ + 0x3f ) & ~0x3f ) ;

    char *a = (char*)aligned_alloc( 64 , ARRAY_LEN ) ;
    char *b = (char*)aligned_alloc( 64 , ARRAY_LEN ) ;
    if( ( (uintptr_t)b & 0x1f ) == 0 ) b += 0x10 ;
    // if( ( (uintptr_t)b & 0x3f ) == 0 ) b += 0x20 ;
    printf( "a @ %p,  b @ %p\n" , a , b ) ;
    for( size_t i = 0 ; i < ARRAY_LEN ; i ++ ) a[i] = i ; 
    for( size_t i = 0 ; i < ARRAY_LEN ; i += 4096 ) b[i] = 0 , c[i] = 2 ;

    for( size_t len = 64 , warmup = 1 ; len <= 8 * MB ; len *= 2 ){
        double st_time , ed_time , alloc_time , submit_time , do_time ;  
        
        // single op
        double DSA_alloc = 0 , DSA_do = 0 ;
        double DSA_time = 0 , DSA_speed = 0 ;
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){
            st_time = timeStamp_hires() ;
            coro_t::pull_type memmove_yield(
                [&]( coro_t::push_type &yield_func ){
                    dsa_memmove_worker worker( [&yield_func](){ yield_func() ; } ,
                    b , a , len ) ;
                    worker.run() ;
                }
            ) ;
            ed_time = timeStamp_hires() , alloc_time = ed_time - st_time , st_time = ed_time ;
            // 创建 coro 和 DSAmem结构 大约2微秒, yield一次大约10ns 
            memmove_yield() ; 
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            DSA_alloc += ( alloc_time ) / REPEAT ;  
            DSA_do += ( do_time ) / REPEAT ;  
        }
        DSA_alloc *= ns_to_us , DSA_do *= ns_to_us ;
        DSA_time = DSA_alloc + DSA_do ;
        DSA_speed = len / ( DSA_time * us_to_s ) / MB ;

        // for( int i = 0 ; i < ARRAY_LEN ; i ++ ) c[i] = b[i] + c[ARRAY_LEN-1-i] ; // flush cache
        double memcpy_time = 0 , memcpy_speed = 0 ;
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){
            st_time = timeStamp_hires() ;  
            for( int i = 0 ; i < tdesc ; i ++ ) memcpy( b + ( 1LL * i * len ) % ARRAY_LEN , a + ( 1LL * i * len ) % ARRAY_LEN , len ) ; 
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
                    b , a , len , bsiz , tdesc , ARRAY_LEN ) ;
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

        double multi_single_do = 0 ;
        double multi_single_time = 0 , multi_single_speed = 0 ;
        DSAop memcpys[MEMCPY_CNT] ; 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){
            st_time = timeStamp_hires() ;
            for( int i = 0 ; i < tdesc ; i ++ ){
                int id = i % MEMCPY_CNT ;
                memcpys[id].wait() ;
                memcpys[id].async_memcpy( b + ( 1LL * i * len ) % ARRAY_LEN , a + ( 1LL * i * len ) % ARRAY_LEN , len ) ;
            }
            for( int i = 0 ; i < MEMCPY_CNT ; i ++ ) memcpys[i].wait() ;
            ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
            multi_single_do += ( do_time ) / REPEAT / tdesc ;
        }
        multi_single_do *= ns_to_us ;
        multi_single_time = multi_single_do ;
        multi_single_speed = len / ( multi_single_time * us_to_s ) / MB ;

        double pure_batch_do = 0 , pure_batch_submit = 0 ;
        double pure_batch_time = 0 , pure_batch_speed = 0 ;
        DSAbatch xfer( bsiz , DEFAULT_BATCH_CAPACITY ) ; 
        for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){ 
            xfer.db_task.clear() ;
            st_time = timeStamp_hires() ;
            for( int i = 0 ; i < tdesc ; i ++ ) xfer.submit_memcpy( b + ( 1LL * i * len ) % ARRAY_LEN , a + ( 1LL * i * len ) % ARRAY_LEN , len ) ;
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
 
        printf( "Copy %s * %d desc, tot %s\n" , stdsiz( len ).c_str() , tdesc , stdsiz( len * tdesc ).c_str() ) ;
        printf( "    ; memcpy     cost %5.2lfus, speed %5.0fMB/s\n"
                "    ; DSA_single cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n" 
                "    ;            |- allc %7.2lfus\n" 
                "    ;            |- wait %7.2lfus\n"
                "    ; DSA_batch  cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n"
                "    ;            |- allc %7.2lfus\n"  
                "    ;            |- subm %7.2lfus\n" 
                "    ;            |- wait %7.2lfus\n"
                "    ; %-3d single cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n"
                "    ; pure_batch cost %5.2lfus, speed %5.0fMB/s, gains %.2lfus\n" 
                "    ;            |- subm %7.2lfus\n" 
                "    ;            |- wait %7.2lfus\n"
                "\n", 
                memcpy_time , memcpy_speed ,
                DSA_time , DSA_speed , memcpy_time - DSA_time , 
                DSA_alloc , 
                DSA_do ,
                DSA_batch_time , DSA_batch_speed , memcpy_time - DSA_batch_time ,
                DSA_batch_alloc , 
                DSA_batch_submit ,
                DSA_batch_do ,
                MEMCPY_CNT , multi_single_time , multi_single_speed , memcpy_time - multi_single_time ,
                pure_batch_time , pure_batch_speed , memcpy_time - pure_batch_time ,
                pure_batch_submit , 
                pure_batch_do ) ; 
        fflush( stdout ) ;  
    } 
}

// g++ coroutine2_dsa.cpp -o coroutine2_dsa -lboost_coroutine -lboost_context -laccel-config -Wall -mcmodel=large