#include "src/async_dsa.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
using namespace std ;

int element_cnt = 0 ; 
int min_len = 0 , max_len = 0 ; // string length
long long sum_len = 0 ;

struct Data {
    int len ;
    char* data ;
    Data() : len( 0 ) , data( nullptr ) {}
} ;

std::vector<Data> genData( int element_cnt , int min_len , int max_len ){
    std::vector<Data> unsorted_data( element_cnt ) ;
    for( int i = 0 ; i < element_cnt ; i ++ ){
        Data &d = unsorted_data[i] ;
        d.len = min_len + rand() % ( max_len - min_len + 1 ) ;
        d.data = new char[d.len + 1] ;
        for( int j = 0 ; j < d.len ; j ++ ){
            d.data[j] = 'a' + rand() % 26 ;
        }
        d.data[d.len] = '\0'; 
    }
    puts("generate finished") ; fflush( stdout ) ;
    return unsorted_data ;
}

vector<Data> sort_by_std( std::vector<Data> &unsorted_data ){
    uint64_t startns = timeStamp_hires() ;
    vector<Data> sorted_data ;
    sorted_data = unsorted_data ;
    std::sort( sorted_data.begin() , sorted_data.end() , []( const Data &a , const Data &b ){
        return strcmp( a.data , b.data ) < 0 ;
    } ) ;
    char* now_data = new char[sum_len] ;
    for( int i = 0 , lim = sorted_data.size() ; i < lim ; i ++ ){
        memcpy( now_data , sorted_data[i].data , sorted_data[i].len + 1 ) ;
        sorted_data[i].data = now_data ;
        now_data += sorted_data[i].len + 1 ;
    } 
    uint64_t endns = timeStamp_hires() ;
    printf( "sort_by_ptr: %ld ns\n" , endns - startns ) ; fflush( stdout ) ;
    printf( "size: %ld\n" , sorted_data.size() ) ; fflush( stdout ) ;
    return sorted_data ;
}

vector<Data> sort_by_ptr( std::vector<Data> &unsorted_data ){ 
    uint64_t startns = timeStamp_hires() ;
    vector<Data> now_data( unsorted_data.size() ) , pre_data( unsorted_data.size() ) ; 
    // init 
    for( int i = 0 , lim = unsorted_data.size() ; i < lim ; i ++ )
        pre_data[i] = unsorted_data[i] ;

    // merge sort
    for( int seglen = 1 ; seglen + 1 < (int)unsorted_data.size() ; seglen *= 2 ){
        // printf( "sorting seglen = %d\n" , seglen ) ; fflush( stdout ) ;
        int now_Data_pos = 0 ; 
        for( int i = 0 , lim = unsorted_data.size() ; i < lim ; i += seglen * 2 ){ 
            int Lpos = i ;
            int Rpos = std::min( i + seglen , (int)unsorted_data.size() ) ;
            int Lposend = Rpos ;
            int Rposend = std::min( i + seglen * 2 , (int)unsorted_data.size() ) ;

            while( Lpos < Lposend && Rpos < Rposend ){
                if( strcmp( pre_data[Lpos].data , pre_data[Rpos].data ) < 0 ){
                    now_data[now_Data_pos] = pre_data[Lpos] ;
                    now_Data_pos ++ ;
                    Lpos ++ ;
                } else {
                    now_data[now_Data_pos] = pre_data[Rpos] ;
                    now_Data_pos ++ ;
                    Rpos ++ ;
                }
            }
            while( Lpos < Lposend ){
                now_data[now_Data_pos] = pre_data[Lpos] ;
                now_Data_pos ++ ;
                Lpos ++ ;
            }
            while( Rpos < Rposend ){
                now_data[now_Data_pos] = pre_data[Rpos] ;
                now_Data_pos ++ ;
                Rpos ++ ;
            } 
        } 
        std::swap( now_data , pre_data ) ;
    } 
    
    char* now = new char[sum_len] ;
    for( int i = 0 , lim = pre_data.size() ; i < lim ; i ++ ){
        memcpy( now , pre_data[i].data , pre_data[i].len + 1 ) ;
        pre_data[i].data = now ;
        now += pre_data[i].len + 1 ;
    } 
    uint64_t endns = timeStamp_hires() ;
    printf( "sort_by_ptr: %ld ns\n" , endns - startns ) ; fflush( stdout ) ;
    return pre_data ;
}

vector<Data> sort_by_dsa( std::vector<Data> &unsorted_data ){
    DSAbatch dsa_batch ;
    uint64_t startns = timeStamp_hires() ; 
    char* now = new char[sum_len] , *pre = new char[sum_len] ;
    vector<Data> now_data( unsorted_data.size() ) , pre_data( unsorted_data.size() ) ; 
    // init
    long long pos = 0 ;
    for( uint64_t x = 0 ; x < (uint64_t)sum_len ; x += 4096 ){
        *( (char*)now + x ) = 0 ;
        *( (char*)pre + x ) = 0 ;
    }
    for( int i = 0 , lim = unsorted_data.size() ; i < lim ; i ++ ){
        pre_data[i].len = unsorted_data[i].len ;
        pre_data[i].data = pre + pos ;
        dsa_batch.submit_memmove( pre + pos , unsorted_data[i].data , unsorted_data[i].len ) ;
        pre[pos + unsorted_data[i].len] = '\0' ;
        pos += unsorted_data[i].len + 1 ;
    }
    dsa_batch.wait() ;

    // merge sort
    for( int seglen = 1 ; seglen + 1 < (int)unsorted_data.size() ; seglen *= 2 ){
        // printf( "sorting seglen = %d\n" , seglen ) ; fflush( stdout ) ;
        int now_Data_pos = 0 ;
        long long now_str_pos = 0 ;
        for( int i = 0 , lim = unsorted_data.size() ; i < lim ; i += seglen * 2 ){ 
            int Lpos = i ;
            int Rpos = std::min( i + seglen , (int)unsorted_data.size() ) ;
            int Lposend = Rpos ;
            int Rposend = std::min( i + seglen * 2 , (int)unsorted_data.size() ) ;

            while( Lpos < Lposend && Rpos < Rposend ){
                if( strcmp( pre_data[Lpos].data , pre_data[Rpos].data ) < 0 ){
                    now_data[now_Data_pos].len = pre_data[Lpos].len ; 
                    dsa_batch.submit_memmove( now + now_str_pos , pre_data[Lpos].data, pre_data[Lpos].len + 1 ) ;
                    now_data[now_Data_pos].data = now + now_str_pos ;
                    now_str_pos += pre_data[Lpos].len + 1 ;
                    now_Data_pos ++ ;
                    Lpos ++ ;
                } else {
                    now_data[now_Data_pos].len = pre_data[Rpos].len ; 
                    dsa_batch.submit_memmove( now + now_str_pos , pre_data[Rpos].data, pre_data[Rpos].len + 1 ) ;
                    now_data[now_Data_pos].data = now + now_str_pos ;
                    now_str_pos += pre_data[Rpos].len + 1 ;
                    now_Data_pos ++ ;
                    Rpos ++ ;
                }
            }
            while( Lpos < Lposend ){
                now_data[now_Data_pos].len = pre_data[Lpos].len ; 
                dsa_batch.submit_memmove( now + now_str_pos , pre_data[Lpos].data, pre_data[Lpos].len + 1 ) ;
                now_data[now_Data_pos].data = now + now_str_pos ;
                now_str_pos += pre_data[Lpos].len + 1 ;
                now_Data_pos ++ ;
                Lpos ++ ;
            }
            while( Rpos < Rposend ){
                now_data[now_Data_pos].len = pre_data[Rpos].len ; 
                dsa_batch.submit_memmove( now + now_str_pos , pre_data[Rpos].data, pre_data[Rpos].len + 1 ) ;
                now_data[now_Data_pos].data = now + now_str_pos ;
                now_str_pos += pre_data[Rpos].len + 1 ;
                now_Data_pos ++ ;
                Rpos ++ ;
            } 
        }
        dsa_batch.wait() ;
        std::swap( now , pre ) ;
        std::swap( now_data , pre_data ) ;
    }
    uint64_t endns = timeStamp_hires() ;
    printf( "sort_by_dsa: %ld ns\n" , endns - startns ) ; fflush( stdout ) ;
    return pre_data ;
}

int main( int argc , char **argv ){
    if( argc < 4 ){
        printf( "Usage: %s <element_cnt> <min_len> <max_len>\n" , argv[0] ) ;
        return 0 ;
    } 
    element_cnt = atoi( argv[1] ) ;
    min_len = atoi( argv[2] ) ;
    max_len = atoi( argv[3] ) ;
 
    vector<Data> unsorted_data = genData( element_cnt , min_len , max_len ) ; 
    for( int i = 0 , lim = unsorted_data.size() ; i < lim ; i ++ ){ 
        sum_len += unsorted_data[i].len + 1 ;
    }
    vector<Data> ptr_sorted_data = sort_by_ptr( unsorted_data ) ;
    vector<Data> dsa_sorted_data = sort_by_dsa( unsorted_data ) ;
    printf( "sum_len: %lld\n" , sum_len ) ; fflush( stdout ) ;
    printf( "ptr_sorted_data: %ld\n" , ptr_sorted_data.size() ) ; fflush( stdout ) ;
    printf( "dsa_sorted_data: %ld\n" , dsa_sorted_data.size() ) ; fflush( stdout ) ;

    for( int i = 0 , lim = unsorted_data.size() ; i < lim ; i ++ ){
        if( strncmp( ptr_sorted_data[i].data , dsa_sorted_data[i].data , 5 ) != 0 ){
            printf( "Error: %d |  ptr:  %s |  dsa: %s\n" , i , ptr_sorted_data[i].data , dsa_sorted_data[i].data ) ;
            continue ;
        } else {
            // printf( "%d %s\n" , i , ptr_sorted_data[i].data ) ;
        }
    } 
    return 0 ;
}