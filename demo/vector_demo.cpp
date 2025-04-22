#include <cstdio>
#include <vector>
#include <cstring>
#include "src/async_dsa.hpp"
using namespace std ;

int cnt = 0 ;

constexpr size_t SINGLE_EXPAND_SIZE = (8*KB) ;
template<typename T>
class vector_DSA{
static_assert(std::is_trivially_copyable<T>::value ,
    "T must be trivially copyable for efficient memcpy operations");
static_assert( sizeof( T ) < SINGLE_EXPAND_SIZE ,
    "T must be smaller than 8KB" ) ; 

private :
    size_t vector_cap ;
    size_t vector_len ;
    size_t old_cap ;
    bool is_expanding ;
    size_t expanding_pos ; // 已经将 expanding_pos - 1 个元素复制到新的内存中
    T* data ;
    T* old_data ;
    DSAbatch dsa_batch ;
    void expand_to( size_t x ) {
        if( expanding_pos > x ) return ;
        while( expanding_pos <= x ){
            size_t nxt_pos = expanding_pos + SINGLE_EXPAND_SIZE / sizeof(T) ;
            cnt ++ ;
            if( nxt_pos > old_cap ){
                nxt_pos = old_cap ;
                dsa_batch.submit_memcpy( data + expanding_pos , old_data + expanding_pos , (nxt_pos - expanding_pos) * sizeof(T) ) ;
                dsa_batch.wait() ;
                delete[] old_data ;
                old_data = nullptr ; 
                is_expanding = false ;
                expanding_pos = 0 ; 
                break ;
            }
            // char* st = (char*)data + expanding_pos * sizeof(T) ;
            // char* ed = (char*)data + nxt_pos * sizeof(T) ;
            // *( ed - 1 ) = 0 ;
            // for( char* i = st ; i < ed ; i += 4096 ) *((char*)i) = 0 ;
            dsa_batch.submit_memcpy( data + expanding_pos , old_data + expanding_pos , (nxt_pos - expanding_pos) * sizeof(T) ) ;
            expanding_pos = nxt_pos ;
        }
    } 
public :
    vector_DSA() : dsa_batch( DEFAULT_BATCH_SIZE , DEFAULT_BATCH_CAPACITY ){
        vector_cap = 4 ;
        vector_len = 0 ;
        is_expanding = false ;
        expanding_pos = 0 ;
        old_cap = 0 ;
        data = new T[vector_cap] ;
        old_data = nullptr ;
    }
    ~vector_DSA() {
        delete[] data;
    }

    void push_back( const T &x ){ 
        if( is_expanding ) expand_to( ( vector_len + 1 ) / 2 ) ; 
        if( vector_len == vector_cap ) {
            // printf( "expanding %d\n" , vector_cap ) ; fflush( stdout ) ;
            old_data = data ;
            old_cap = vector_cap ;
            vector_cap *= 2 ; 
            data = new T[vector_cap] ;
            if( old_cap <= 64 * KB / sizeof( T ) ){
                memcpy( data , old_data , old_cap * sizeof( T ) ) ;
                delete[] old_data ;
            } else {
                is_expanding = true ;
                expanding_pos = 0 ;
                int lim = old_cap * sizeof( T ) ;
                for( int i = 0 ; i < lim ; i += 4096 ){
                    *( (char*)data + i ) = 0 ;
                }
            }
        }
        data[vector_len++] = x;
    }

    void pop_back(){
        if( is_expanding ) expand_to( expanding_pos ) ;
        if( vector_len > 0 ) vector_len -- ;
    }

    T& operator [](size_t index) { 
        if( is_expanding && index < old_cap ) finish_expand() ;
        return data[index];
    } 

    void    finish_expand(){ if( is_expanding ) expand_to( old_cap ) ; }
    size_t  capacity() const { return vector_cap; }
    size_t  size() const { return vector_len; } 
    void    clear() { vector_len = 0 ; }
} ;

int main(){
    vector<uint64_t> std_vector ; 

    uint64_t startns = timeStamp_hires() ;
    vector_DSA<uint64_t> my_vector;
    // vector<uint64_t> my_vector;
    for( int i = 0 ; i < 134217728 ; i ++ ){
        my_vector.push_back( i ) ; 
    }


    printf( "capacity = %lu , size = %lu\n" , my_vector.capacity() , my_vector.size() ) ;
    uint64_t endns = timeStamp_hires() ;
    printf( "cost time = %.5fms, submit_cnt = %d\n" , ( endns - startns ) / 1000000.0 , cnt ) ;

    // check correctness
    for( uint64_t i = 0 ; i < my_vector.size() ; i ++ ){
        if( my_vector[i] != i ) {
            printf( "Error at %lu, value is %lu\n" , i , my_vector[i] ) ;
            return 1 ;
        }
    }
    printf( "All elements are correct\n" ) ;
    endns = timeStamp_hires() ;
    printf( "cost time = %.5fms\n" , ( endns - startns ) / 1000000.0 ) ;
}