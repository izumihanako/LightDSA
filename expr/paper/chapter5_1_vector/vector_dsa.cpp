#include <cstdio>
#include <vector>
#include <cstring>
#include "src/async_dsa.hpp"
using namespace std ;

int cnt = 0 , REPEAT = 10 ;

constexpr size_t SINGLE_EXPAND_SIZE = (128*KB) ; 

template<typename T>
class vector_DSA{
static_assert(std::is_trivially_copyable<T>::value ,
    "T must be trivially copyable for efficient memcpy operations");
static_assert( sizeof( T ) < SINGLE_EXPAND_SIZE ,
    "T must be smaller than 8KB" ) ; 

public : 
    size_t vector_cap ;
    size_t vector_len ; 
    T* data ;
    int method ;
    DSAbatch dsa_batch ;
    DSAop dsa_op ;
    const size_t default_expand_step = SINGLE_EXPAND_SIZE / sizeof( T ) ;
    const size_t default_expand_size = SINGLE_EXPAND_SIZE / sizeof( T ) * sizeof( T ) ; 

    __always_inline void dsa_expand( T* data , T* old_data , size_t old_cap ) { 
        size_t expanding_pos = 0 ; // 已经将 expanding_pos - 1 个元素复制到新的内存中
        while( true ){ 
            size_t nxt_pos = expanding_pos + default_expand_step ;
            if( nxt_pos >= old_cap ){
                nxt_pos = old_cap ;
                dsa_batch.submit_memcpy( data + expanding_pos , old_data + expanding_pos , (nxt_pos - expanding_pos) * sizeof(T) ) ;
                dsa_batch.wait() ; 
                break ;
            } else {
                dsa_batch.submit_memcpy( data + expanding_pos , old_data + expanding_pos , default_expand_size ) ;
                expanding_pos = nxt_pos ;
            }
        }
    }  
public :
    vector_DSA( int method_ ) : dsa_batch( DEFAULT_BATCH_SIZE , DEFAULT_BATCH_CAPACITY ){
        vector_cap = 4 ;
        vector_len = 0 ; 
        data = (T*) malloc( vector_cap * sizeof( T ) ) ;
        method = method_ ;
    }
    ~vector_DSA() {
        delete[] data; 
    }

    void push_back( const T &x ){
        if( vector_len == vector_cap ) { 
            T* new_data = (T*) malloc( vector_cap * 2 * sizeof( T ) ) ;
            // uint64_t startns , endns ;
            // startns = timeStamp_hires() ;
            if( method == 0 ){ // use memcpy 
                memcpy( new_data , data , vector_cap * sizeof( T ) ) ;
            } else if( method == 1 ){
                dsa_op.sync_memcpy( new_data , data , vector_cap * sizeof( T ) ) ;
            } else if( method == 2 ){ // use DSA with memcpy
                if( vector_cap <= 512 * KB / sizeof( T ) ){ // for cache locality
                    memcpy( new_data , data , vector_cap * sizeof( T ) ) ; 
                } else {
                    dsa_expand( new_data , data , vector_cap ) ;
                }
            } 
            // endns = timeStamp_hires() ;
            // printf( "vector_DSA : cost time = %.3fs, vector_cap = %ld, speed = %.3fGB/s\n" , ( endns - startns ) / 1000000000.0 , vector_cap , 1.0 * vector_cap * sizeof( T ) / GB / ( ( endns - startns ) / 1000000000.0 )  ) ; fflush( stdout ) ;
            free( data ) ;
            data = new_data ;
            vector_cap *= 2 ;
        } 
        data[vector_len++] = x ;
    } 

    T& operator [](size_t index) {
        return data[index];
    } 
 
    __always_inline size_t capacity() const { return vector_cap; }
    __always_inline size_t size() const { return vector_len; } 
    void    clear() { vector_len = 0 ; }
} ;

int operation_cnt = 100000000 ;
double put_get_percent[] = { 1.0 , 5.0 , 25.0 , 50.0 , 75.0 , 95.0 , 99.0 , 100.0 } ;
vector<int> rand_sequence , idx_sequence ;

void test_vector_speed( double ratio ) { 
    double cpu_time = 0 , naive_time = 0 , my_time = 0 ;  
    int size = 0 ;
    for( int i = 0 , warmup = 0 ; i < REPEAT ; i ++ ){
        vector<uint64_t> std_vector ;
        // vector_DSA<uint64_t> std_vector( 0 ) ;
        uint64_t cpu_startns = timeStamp_hires() ;
        std_vector.push_back( 0 ) ; 
        uint64_t x = 0 ;
        for( int i = 0 ; i < operation_cnt ; i ++ ){
            int rand_val = rand_sequence[i] ;
            if( 1.0 * rand_val / RAND_MAX < ratio ) {
                std_vector.push_back( i ^ x ) ; 
            } else {
                x = std_vector[rand_val % std_vector.size()] ;
            }
        }
        uint64_t cpu_endns = timeStamp_hires() ; 
     
        vector_DSA<uint64_t> naive_dsa_vector( 1 ) ;
        uint64_t naive_startns = timeStamp_hires() ;
        naive_dsa_vector.push_back( 0 ) ;
        x = 0 ;
        for( int i = 0 ; i < operation_cnt ; i ++ ){
            int rand_val = rand_sequence[i] ;
            if( 1.0 * rand_val / RAND_MAX < ratio ) {
                naive_dsa_vector.push_back( i ^ x ) ; 
            } else {
                x = naive_dsa_vector[rand_val % naive_dsa_vector.size()] ;
            }
        }
        uint64_t naive_endns = timeStamp_hires() ; 
 
        vector_DSA<uint64_t> my_dsa_vector( 2 ) ;
        uint64_t my_startns = timeStamp_hires() ;
        my_dsa_vector.push_back( 0 ) ;
        x = 0 ;
        for( int i = 0 ; i < operation_cnt ; i ++ ){
            int rand_val = rand_sequence[i] ;
            if( 1.0 * rand_val / RAND_MAX < ratio ) {
                my_dsa_vector.push_back( i ^ x ) ; 
            } else {
                x = my_dsa_vector[rand_val % my_dsa_vector.size()] ;
            }
        }
        uint64_t my_endns = timeStamp_hires() ;

        // check correctness 
        for( uint64_t i = 0 ; i < my_dsa_vector.size() ; i ++ ){
            if( my_dsa_vector[i] != std_vector[i] || naive_dsa_vector[i] != std_vector[i] ){
                printf( "Error at %lu, std is %lu, value is %lu,%lu \n" , i , std_vector[i] , naive_dsa_vector[i] , my_dsa_vector[i] ) ;
                getchar() ;
            }
        }
        if( warmup == 0 ){ warmup ++ ; i -- ; continue ; }
        cpu_time += ( cpu_endns - cpu_startns ) / 1000000000.0 / REPEAT ;
        naive_time += ( naive_endns - naive_startns ) / 1000000000.0 / REPEAT ;
        my_time += ( my_endns - my_startns ) / 1000000000.0 / REPEAT ;
        size = (int)std_vector.size() ;
    } 
    printf( "vector test : put_ratio = %.3f%%, vector size = %d, REPEAT = %d\n" , ratio * 100 , size , REPEAT ) ;
    printf( "std_vector     : cost time = %.3fs, operation_cnt = %d\n" , cpu_time , operation_cnt ) ; fflush( stdout ) ;
    printf( "nve_dsa_vector : cost time = %.3fs, operation_cnt = %d\n" , naive_time , operation_cnt ) ; fflush( stdout ) ;
    printf( "my_dsa_vector  : cost time = %.3fs, operation_cnt = %d\n" , my_time , operation_cnt ) ; fflush( stdout ) ;   
}

DSAop __ ;
int main(){ 
    srand( 0 ) ; 
    for( int i = 0 ; i < operation_cnt ; i ++ ) rand_sequence.push_back( rand() ) ;  
    for( int i = 0 ; i < (int)( sizeof( put_get_percent ) / sizeof( double ) ) ; i ++ ){
        double put_get_ratio = put_get_percent[i] / 100 ;  
        test_vector_speed( put_get_ratio ) ;
    }
}