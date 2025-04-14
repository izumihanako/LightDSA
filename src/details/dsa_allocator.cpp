#include "dsa_allocator.hpp"
#include "util.hpp"
#include <cstdio> 
#include <cstring>
#include <sys/mman.h>
#include <cassert>

DSAallocator::DSAallocator( int pool_size_ ) {
    pool_size = pool_size_ ;
    memory_pool = MAP_FAILED ;
    node = nullptr ;
    
    #ifdef ALLOCATOR_USE_HUGEPAGE
    // try to malloc 2M page
    memory_pool = mmap( NULL , pool_size , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;
    if( memory_pool == MAP_FAILED ) {
        printf( "Failed to mmap 2M pages for DSA allocator, try to mmap 4K pages\n" ) ;
        memory_pool = mmap( NULL , pool_size , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS , -1 , 0 ) ;
        if( memory_pool == MAP_FAILED ) {
            printf( "Failed to mmap for DSA allocator.\n" ) ;
            exit( -1 ) ;
        }
    }
    #else 
    // try to malloc 4K page
    memory_pool = mmap( NULL , pool_size , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS , -1 , 0 ) ;
    if( memory_pool == MAP_FAILED ) {
        printf( "Failed to mmap for DSA allocator.\n" ) ;
        exit( -1 ) ;
    }
    #endif
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
    if( memory_pool != MAP_FAILED ) munmap(memory_pool, pool_size);
}

void* DSAallocator::allocate( size_t size ){ 
    if( size == 0 ) return nullptr ;
    if( size & 0x1f ) {
        printf( "Allocate size is aligned: %ld -> %ld\n" , size , ( size + 31 ) & ~0x1f ) ;
        size = ( size + 31 ) & ~0x1f ;
    }
    void *rt = _alloc( 1 , 0 , pool_size - 1 , 0 , 0 , (int)size , false ) ;
    if( rt == nullptr ) {
        printf( "Allocate %lu Bytes Failed, no enough memory\n" , size ) ;
        return nullptr ;
    }
    int pos = ( (char*)rt - (char*)memory_pool ) / 32 ;
    node[leaf_cnt-1+pos].this_len = size / 32 ;
    return rt;
}

void DSAallocator::deallocate( void* ptr ){
    if( ptr == nullptr ) return ;
    if( (char*)ptr < (char*)memory_pool || (char*)ptr >= (char*)memory_pool + pool_size ) {
        printf( "Deallocate Failed, pointer out of range\n" ) ;
        return ;
    }
    int pos = ( (char*)ptr - (char*)memory_pool ) / 32 ;
    if( pos < 0 || pos >= leaf_cnt ) {
        printf( "Deallocate Failed, pointer out of range\n" ) ;
        return ;
    }
    int L = pos * 32 , R = L + node[leaf_cnt-1+pos].this_len * 32 - 1 ; 
    _dealloc( 1 , 0 , pool_size - 1 , L , R ) ;
}

void DSAallocator::print_space(){
    _bfs_on_tree() ;
}

void* DSAallocator::_alloc( int node_id , int lf , int rg , int L , int R , int size , bool finded ) {
    // printf( "_alloc N%d[%d %d] , L = %d , R = %d\n" , node_id , lf , rg , L , R ) ;
    if( finded ){
        if( L <= lf && rg <= R ){
            mark_node( node_id , true , rg - lf + 1 ) ;
            return (char*)memory_pool + lf ;
        }
        int mid = ( lf + rg ) >> 1 ;
        push_down( node_id , lf , rg ) ;
        void *retL = nullptr , *retR = nullptr ;
        if( L <= mid ) retL = _alloc( node_id * 2 , lf , mid , L , R , size , true ) ;
        if( R >  mid ) retR = _alloc( node_id*2+1 , mid + 1 , rg , L , R , size , true ) ;
        update_node( node_id , lf , rg ) ;
        return retL ? retL : retR ;
    } else {
        if( node[node_id].len < size ) return nullptr ;
        int mid = ( lf + rg ) >> 1 ;
        push_down( node_id , lf , rg ) ;
        // 先尝试分配左边
        if( node[node_id * 2].len >= size ) {
            void *ret = _alloc( node_id * 2 , lf , mid , 0 , 0 , size , false ) ; 
            update_node( node_id , lf , rg ) ;
            if( ret != nullptr ) return ret ;
            else {
                printf( "Allocate Seg Tree Failed, len >= size but nullptr\n" ) ;
                exit( -1 ) ;
            }
        }
        // 然后看中间的部分有没有
        if( node[node_id * 2].r_len + node[node_id * 2 + 1].l_len >= size ) { 
            int Lpos = mid - node[node_id * 2].r_len + 1 ;
            int Rpos = Lpos + size - 1 ;
            void *retL = nullptr , *retR = nullptr ;
            if( Lpos == lf && Rpos == rg ) {
                mark_node( node_id , true , rg - lf + 1 ) ;
                return (char*)memory_pool + lf ;
            }
            if( Lpos <=mid ) retL = _alloc( node_id * 2 , lf , mid , Lpos , mid , size , true ) ;
            if( Rpos > mid ) retR = _alloc( node_id*2+1 , mid + 1 , rg , mid + 1 , Rpos , size , true ) ;
            update_node( node_id , lf , rg ) ;
            if( !retL && !retR ) {
                printf( "Allocate Seg Tree Failed, mid len >= size but nullptr\n" ) ;
                exit( -1 ) ;
            }
            return retL ? retL : retR ; 
        }
        // 最后尝试分配右边
        if( node[node_id * 2 + 1].len >= size ) {
            void *ret = _alloc( node_id * 2 + 1 , mid + 1 , rg , 0 , 0 , size , false ) ;
            update_node( node_id , lf , rg ) ;
            if( ret != nullptr ) return ret ;
            else {
                printf( "Allocate Seg Tree Failed, len >= size but nullptr\n" ) ;
                exit( -1 ) ;
            }
        }
        return nullptr ;
    }
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