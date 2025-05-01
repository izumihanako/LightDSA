#include "dsa_allocator.hpp"
#include "util.hpp"
#include <cstdio> 
#include <cstring>
#include <sys/mman.h>
#include <cassert>

DSAallocator::DSAallocator( int pool_size_ ) {
    if( pool_size_ & ( pool_size_ - 1 ) ) {
        printf( "pool_size must be power of 2\n" ) ; fflush( stdout ) ;
        exit( -1 ) ;
    } 
    pool_size = pool_size_ ;
    memory_pool = MAP_FAILED ;
    node = nullptr ;
    used_size = 0 ;

    void* _memory_pool = nullptr ;

    #ifdef ALLOCATOR_USE_HUGEPAGE
        // try to malloc 2M page
        _memory_pool = mmap( NULL , pool_size * 2 , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;
        if( _memory_pool == MAP_FAILED ) {
            printf( "Failed to mmap 2M pages for DSA allocator, try to mmap 4K pages\n" ) ;
            _memory_pool = mmap( NULL , pool_size * 2 , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS , -1 , 0 ) ;
            if( _memory_pool == MAP_FAILED ) {
                printf( "Failed to mmap for DSA allocator.\n" ) ;
                exit( -1 ) ;
            }
        }
    #else 
        // try to malloc 4K page
        _memory_pool = mmap( NULL , pool_size * 2 , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS , -1 , 0 ) ;
        if( _memory_pool == MAP_FAILED ) {
            printf( "Failed to mmap for DSA allocator.\n" ) ;
            exit( -1 ) ;
        }
    #endif
    // align the memory pool to pool_size bytes
    memory_pool = (void*)( ( (uintptr_t) _memory_pool + pool_size - 1 ) & ~( pool_size - 1 ) ) ;
    size_t len1 = (uintptr_t)memory_pool - (uintptr_t)_memory_pool ; 
    if( len1 ) munmap( _memory_pool , len1 ) ;
    uintptr_t start2 = (uintptr_t)memory_pool + pool_size ;
    size_t len2 = (uintptr_t)_memory_pool + pool_size * 2 - start2 ;
    if( len2 ) munmap( (void*)start2 , len2 ) ; 
    memset( memory_pool , 0 , pool_size ) ; 

    leaf_cnt = pool_size / 32 ;
    // 非叶节点编号为 1 ~ leaf_cnt - 1, 叶节点编号为 leaf_cnt ~ 2*leaf_cnt - 1
    node = (seg_tree_node*)malloc( sizeof(seg_tree_node) * leaf_cnt * 2 ) ;
    if (node == nullptr) {
        printf("Failed to allocate memory for segment tree nodes.\n");
        exit(-1);
    }
    mark_node( 1 , false , pool_size ) ; 
}

DSAallocator::~DSAallocator() {
    if( node != nullptr ) free(node);
    if( memory_pool != MAP_FAILED ) munmap( memory_pool, pool_size );
}

void* DSAallocator::allocate( size_t size , size_t alignment ){ 
    if( size == 0 ) return nullptr ;
    if( alignment & ( alignment - 1 ) ) {
        printf( "Alignment must be power of 2\n" ) ; fflush( stdout ) ;
        return nullptr ;
    } 
    if( size & ( alignment - 1 ) ) {
        printf( "Allocate size is aligned: %ld -> %ld\n" , size , ( size + alignment - 1 ) & ~( alignment - 1 ) ) ; fflush( stdout ) ;
        size = ( size + alignment - 1 ) & ~( alignment - 1 ) ;
    }
    int Lpos = _find( 1 , 0 , pool_size - 1 , size + ( alignment == 32 ? 0 : alignment ) ) ;
    if( Lpos == -1 ) {
        printf( "Allocate %lu Bytes Failed, no enough memory\n" , size ) ; fflush( stdout ) ;
        return nullptr ;
    }
    if( Lpos & ( alignment - 1 ) ) Lpos += alignment - ( Lpos & ( alignment - 1 ) ) ; 
    int Rpos = Lpos + size - 1 ; 
    _alloc( 1 , 0 , pool_size - 1 , Lpos , Rpos ) ; 
    int leaf_offset = Lpos / 32 ;
    node[leaf_cnt-1+leaf_offset].this_len = size / 32 ;
    used_size += size ;
    return (char*)memory_pool + Lpos ;
}

void DSAallocator::deallocate( void* ptr ){
    if( ptr == nullptr ) return ;
    if( (char*)ptr < (char*)memory_pool || (char*)ptr >= (char*)memory_pool + pool_size ) {
        printf( "Deallocate Failed, pointer out of range\n" ) ;
        return ;
    }
    int leaf_offset = ( (char*)ptr - (char*)memory_pool ) / 32 ;
    if( leaf_offset < 0 || leaf_offset >= leaf_cnt ) {
        printf( "Deallocate Failed, pointer out of range\n" ) ;
        return ;
    }
    used_size -= node[leaf_cnt-1+leaf_offset].this_len * 32;
    int L = leaf_offset * 32 , R = L + node[leaf_cnt-1+leaf_offset].this_len * 32 - 1 ; 
    _dealloc( 1 , 0 , pool_size - 1 , L , R ) ;
}

void DSAallocator::print_space(){
    _bfs_on_tree() ;
}

int DSAallocator::_find( int node_id , int lf , int rg , int size ){ 
    if( node[node_id].len < size ) return -1 ;
    if( rg - lf == 32 ){
        if( node[node_id].len >= size ) return lf ;
        else assert( false && "Error in Allocator" ) ;
    }
    int mid = ( lf + rg ) >> 1 ;
    push_down( node_id , lf , rg ) ;
    // 先看左边
    if( node[node_id * 2].len >= size ) return _find( node_id * 2 , lf , mid , size ) ;
    // 再看中间
    if( node[node_id*2].r_len + node[node_id*2+1].l_len >= size ) {
        // Lpos = mid - node[node_id * 2].r_len + 1 ;
        // Rpos = Lpos + size - 1 ; 
        return mid - node[node_id*2].r_len + 1 ;
    } 
    if( node[node_id * 2 + 1].len >= size ) return _find( node_id * 2 + 1 , mid + 1 , rg , size ) ;
    return -1;
}

void DSAallocator::_alloc( int node_id , int lf , int rg , int L , int R ) {  
    if( L <= lf && rg <= R ){
        mark_node( node_id , true , rg - lf + 1 ) ;
        return ;
    }
    int mid = ( lf + rg ) >> 1 ;
    push_down( node_id , lf , rg ) ; 
    if( L <= mid ) _alloc( node_id * 2 , lf , mid , L , R ) ;
    if( R >  mid ) _alloc( node_id*2+1 , mid + 1 , rg , L , R ) ;
    update_node( node_id , lf , rg ) ; 
}

void DSAallocator::_dealloc( int node_id , int lf , int rg , int L , int R ) {
    if( L <= lf && rg <= R ){
        mark_node( node_id , false , rg - lf + 1 ) ;
        return ;
    }
    int mid = ( lf + rg ) >> 1 ;
    push_down( node_id , lf , rg ) ;
    if( L <= mid ) _dealloc( node_id * 2 , lf , mid , L , R ) ;
    if( R >  mid ) _dealloc( node_id * 2 + 1 , mid + 1 , rg , L , R ) ;
    update_node( node_id , lf , rg ) ;
}

void DSAallocator::_bfs_on_tree(){
    int level_cnt = 0 ; 
    bool *will_print = (bool*)malloc( sizeof(bool) * leaf_cnt * 2 ) ;
    memset( will_print , 0 , sizeof(bool) * leaf_cnt * 2 ) ;
    will_print[1] = true ;

    for( int tmp = leaf_cnt ; tmp > 1 ; tmp = tmp / 2 , level_cnt ++ ) ;
    for( int level = 0 ; level <= level_cnt ; level ++ ){
        int start = 1 << level , end = ( 1 << ( level + 1 ) ) - 1 ;
        int lf = 0 , rg = 32 * ( 1 << ( level_cnt - level ) ) - 1 ;
        bool is_break = true ;
        for( int i = start ; i <= end ; i ++ ){
            if( will_print[i] ){
                is_break = false ;
                if( node[i].len == 0 ) {
                    printf_RGB( 255 , 0 , 0 , "N%d[%d-%d] " , i , lf , rg ) ;
                } else if( node[i].len == rg - lf + 1 ) {
                    printf_RGB( 0 , 255 , 0 , "N%d[%d-%d] " , i , lf , rg ) ;
                } else {
                    printf_RGB( 0 , 0 , 255 , "N%d[%d-%d] " , i , lf , rg ) ;
                    will_print[i*2] = true ;
                    will_print[i*2+1] = true ;
                } 
            }
            lf = rg + 1 , rg += 32 * ( 1 << ( level_cnt - level ) ) ;
        }
        puts( "" ) ;
        if( is_break ) break ;
    } 

    free( will_print ) ;
}