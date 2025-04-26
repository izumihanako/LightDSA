#include <boost/coroutine2/all.hpp>
#include "src/async_dsa.hpp"
#include <vector> 
#include <atomic>
#include <cstring>
 
constexpr int coro = 4 ;
constexpr int element_per_co = 5000000 ;
constexpr int group = 8 ;
constexpr int buffer_size = 2048 ;
constexpr int databytes = 1016 ;
constexpr int hashbytes = 8 ;

Galois_LFSR lsfr ; 

struct Data{
    char a[databytes] ;
    int64_t hashv ;
    bool operator == ( const Data &another ) const {
        for( int i = 0 ; i < databytes ; i += 8 ){
            if( *((uint64_t*)(a+i)) != *((uint64_t*) another.a+i) ) return false ;
        }
        return true ;
    }
    int64_t gethash(){
        return hashv != -1 ? hashv : hashv = FNV_hash( a , hashbytes ) % group ;
    }
    void random_fill(){
        for( int i = 0 ; i < databytes ; i += 8 ){
            *((uint64_t*)(a + i)) = lsfr.lfsr64() ;
        }
    }
} ;

struct DataList{
    std::vector<Data> data ;
    std::atomic<size_t> len ; 
    Data& operator []( int x ){
        return data[x] ;
    }
    void clear_hash(){
        for( size_t i = 0 ; i < data.size() ; i ++ ) 
            data[i].hashv = -1 ;
    }
    void random_init( int cnt ) { 
        len.store( 0 ) ;
        data.resize( cnt ) ;
        for( int i = 0 ; i < cnt ; i ++ ){
            data[i].random_fill() ;
            data[i].hashv = -1 ;
        }
    } 
    void resize( int cnt ){
        data.resize( cnt ) ;
    } 
    Data* push_get_addr( size_t delta ){
        size_t pre = len.fetch_add( delta ) ;
        if( pre + delta > data.capacity() ){ assert( false ) ; } 
        return data.data() + pre ;
    }
} before[coro] , after_dsa[group] , after_memcpy[group] ;
 
class shuffle_dsa_worker{
    std::function<void(void)> yield_out ;
    int worker_id , buffer_cnt[group] ;
    std::atomic<int>* buc_cap , *barr ; 
    Data *buffer[group] ;
    DSAmemcpy memcpy_ ; 
    DSAbatch  batch_[group] ;
    DataList* after_dsa ;
    bool useDSA ;

public :
    shuffle_dsa_worker( std::function<void(void)> yield_func_ , int id_ , std::atomic<int>* buc_cap_ , std::atomic<int>* bar_ )
    : yield_out(yield_func_) , worker_id(id_) , buc_cap(buc_cap_) , barr(bar_) {
        memset( buffer_cnt , 0 , sizeof( buffer_cnt ) ) ; 
    }

    void run( bool useDSA_ , void* after_dsa_ ){
        useDSA = useDSA_ , after_dsa = (DataList (*)) after_dsa_ ;

        printf( "this is worker %d!\n" , worker_id ) ; fflush( stdout ) ;
        DataList &src = before[worker_id] ;
        auto     dst = after_dsa ;
        for( int i = 0 ; i < group ; i ++ ){
            buffer[i] = new Data[buffer_size] ;
            // trigger page fault
            for( int siz = sizeof( Data ) * buffer_size , j = 0 ; j <= siz ; j += 4096 ){
                *( ( (char*)buffer[i] ) + j ) = 0 ;
            }
        }

        // count bucket capacity
        for( int i = 0 ; i < element_per_co ; i ++ ){
            buc_cap[ src[i].gethash() ].fetch_add( 1 ) ;
        }
        barr->fetch_sub( 1 ) ;
        while( barr->load() != -1 ){ 
            if( worker_id == 0 && barr->load() == 0 ){
                for( int i = 0 ; i < group ; i ++ ){  
                    dst[i].resize( buc_cap[i] ) ;
                }
                barr->fetch_sub( 1 ) ;
            } else yield_out() ;
        }

        printf( "worker %d go on\n" , worker_id ) ;
        // do copy 
        for( int i = 0 ; i < element_per_co ; i ++ ){
            uint32_t hash = src[i].gethash() ;
            int &cnt = buffer_cnt[hash] ;
            if( i % 1000000 == 0 ) { printf( "worker(%d) : i = %d\n" , worker_id , i ) ; fflush( stdout ) ;}
            if( useDSA ){ 
                batch_[hash].submit_memcpy( &buffer[hash][cnt++] , &src[i] , sizeof( Data ) ) ; 
            } else
                buffer[hash][cnt++] = src[i] ;
            if( cnt == buffer_size ) { 
                offload( hash ) ; 
            }
        } 
        int sum_cnt = 0 ;
        for( int i = 0 ; i < group ; i ++ ) sum_cnt += batch_[i].to_cpu ;
        printf( "worker %d , DSA_memcpy_cnt = %d\n" , worker_id , sum_cnt ) ;
        for( int i = 0 ; i < group ; i ++ ) offload( i ) ;
        for( int i = 0 ; i < group ; i ++ ) delete[] buffer[i] ;
    }

    void offload( int buc_id ){
        while( !batch_[buc_id].check() ) yield_out() ;

        int &cnt = buffer_cnt[buc_id] ;
        void *dest = after_dsa[buc_id].push_get_addr( cnt ) , *src = buffer[buc_id] ;
        size_t len = cnt * sizeof( Data ) ;
        if( useDSA ){
            memcpy_.do_async( dest , src , len ) ;
            while( !memcpy_.check() ){ yield_out() ; }
        } else {
            memcpy( dest , src , len ) ; 
        }
        cnt = 0 ;
    }
} ;

void prepare_data(){ 
    double st_time = timeStamp() ;
    printf( "Preparing data : 0.0%%..." ) ; fflush( stdout ) ;
    for( int i = 0 ; i < coro ; i ++ ){
        before[i].random_init( element_per_co ) ;
        printf( "\rPreparing data : %.1f%%..." , 100.0 * ( i + 1 ) / coro ) ; fflush( stdout ) ;
    } 
    double ed_time = timeStamp() ;
    printf( "\nPrepare cost %.5fs\n\n" , ed_time - st_time ) ;
} 

void shuffle_data_dsa( bool useDSA ) {
    // defs 
    double st_time = timeStamp() ;
    printf( "Shuffle data using %s\n" , useDSA ? "DSA" : "memcpy" ) ; 

    typedef boost::coroutines2::coroutine<void> coro_t ; 
    std::atomic<int> buc_cap[group] , barr ;
    barr.store( coro ) ;
    for( int i = 0 ; i < group ; i ++ ) buc_cap[i].store( 0 ) ; 
    // coro
    std::vector< coro_t::pull_type > coros ;
    for( int i = 0 ; i < coro ; i ++ ){ 
        coros.push_back( coro_t::pull_type( 
            [&]( coro_t::push_type & yield_func ){
                shuffle_dsa_worker worker( [&yield_func](){ yield_func() ; } , i , buc_cap , &barr ) ;
                worker.run( useDSA , useDSA ? after_dsa : after_memcpy ) ;
            }
        ) ) ;
    }
    while( true ){
        bool not_finish = false ;
        for( auto &coro : coros ){
            if( coro.operator bool() ) {
                coro() ;
                not_finish = true ;
            }
        }
        if( !not_finish ) break ;
    }
    double ed_time = timeStamp() ;
    printf( "Shuffle(%s) cost %.5fs\n\n" , useDSA ? "DSA" : "memcpy" , ed_time - st_time ) ;
}

int main(){
    { DSAmemcpy tmp ; }
    // explain && defs 
    printf( "Demo of using libboost::coroutine2 to do data shuffling\n" ) ; 

    // prepare data  
    prepare_data() ; 
    
    // shuffle data using DSA
    for( int i = 0 ; i < coro ; i ++ ) before[i].clear_hash() ;
    shuffle_data_dsa( true ) ;

    // shuffle data using memcpy
    for( int i = 0 ; i < coro ; i ++ ) before[i].clear_hash() ;
    shuffle_data_dsa( false ) ;

    // check  
    printf( "Check shuffle(DSA) result : 0.0%%..." ) ; 
    int sum_len = 0 ;
    for( int i = 0 ; i < group ; i ++ ){
        after_dsa[i].clear_hash() ; 
        sum_len += after_dsa[i].len.load() ;
        for( int j = after_dsa[i].len.load() - 1 ; j >= 0 ; j -- ){
            if( (int)after_dsa[i][j].gethash() != i ){
                printf( "Error on after_dsa : group %d element %d\n" , i , j ) ;
                printf( "val = %lu,  hashv = %ld\n" , (uint64_t)after_dsa[i][j].a , after_dsa[i][j].gethash() ) ;
            }
        }
        printf( "\rCheck shuffle(DSA) result : %.1f%%..." , 100.0 * ( i + 1 ) / group ) ; fflush( stdout ) ;
    } printf( "\nelement cnt = %d\n" , sum_len ) ;
    return 0 ;
}