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
constexpr double ns_to_s = 1.0 / 1000000000.0 ;

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

double prepare_time(0) , calc_hash_time(0) , shuffle_time(0) ; // s
 
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
 
    double ed_time = timeStamp() ;
    printf( "Shuffle(%s) cost %.5fs\n\n" , useDSA ? "DSA" : "memcpy" , ed_time - st_time ) ;
}

int main(){
    { DSAop tmp ; }
    // explain && defs 
    printf( "Demo of using libboost::coroutine2 to do data shuffling\n" ) ; 

    // prepare data  
    prepare_data() ; 
    
    // shuffle data using DSA
    for( int i = 0 ; i < coro ; i ++ ) before[i].clear_hash() ;
    shuffle_data_dsa( true ) ; 

    // shuffle data using memcpy
    prepare_time = 0 , calc_hash_time = 0 , shuffle_time = 0 ;
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