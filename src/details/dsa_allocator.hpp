#ifndef QWQ_DSA_ALLOCATOR_HPP
#define QWQ_DSA_ALLOCATOR_HPP

#include "dsa_util.hpp"
#include "dsa_conf.hpp"
#include <algorithm>


class DSAallocator{
public :
    DSAallocator( int pool_size_ = DEFAULT_POOL_SIZE ) ;

    ~DSAallocator() ;
 
    void* allocate( size_t size ) ;

    void deallocate( void* ptr ) ;

    void print_space() ;

private :
    void* memory_pool ;
    size_t pool_size ;

private : 
    struct seg_tree_node{
        uint32_t l_len , r_len , len ;
        int     this_len : 30 ;
        bool    push_down : 1 ;
        bool    pd_flag : 1 ;
    } ;

    __always_inline void mark_node( int node_id , bool pd_flag , int node_len ){
        node[node_id].pd_flag = pd_flag ;
        node[node_id].push_down = true ;
        if( pd_flag ){
            node[node_id].l_len = node[node_id].r_len = node[node_id].len = 0 ;
        } else {
            node[node_id].l_len = node[node_id].r_len = node[node_id].len = node_len ;
        }
    }

    __always_inline void push_down( int node_id , int lf , int rg ){
        if (node[node_id].push_down) {
            int mid = ( lf + rg ) >> 1 ;
            mark_node( node_id * 2 , node[node_id].pd_flag , mid - lf + 1 ) ;
            mark_node( node_id * 2 + 1 , node[node_id].pd_flag , rg - mid ) ; 
            node[node_id].push_down = false;
        }
    }

    __always_inline void update_node( int node_id , int lf , int rg ){
        int mid = ( lf + rg ) >> 1 ;
        node[node_id].l_len = node[node_id * 2].l_len ;
        if( node[node_id * 2].l_len == mid - lf + 1 ) node[node_id].l_len += node[node_id * 2 + 1].l_len ;
        node[node_id].r_len = node[node_id * 2 + 1].r_len ;
        if( node[node_id * 2 + 1].r_len == rg - mid ) node[node_id].r_len += node[node_id * 2].r_len ;
        
        node[node_id].len = std::max( node[node_id * 2].len , node[node_id * 2 + 1].len ) ;
        uint32_t mid_len = node[node_id * 2].r_len + node[node_id * 2 + 1].l_len ;
        if( mid_len > node[node_id].len ) node[node_id].len = mid_len ;
    }

    void *_alloc( int node_id , int lf , int rg , int L , int R , size_t size , bool finded ) ;

    void _dealloc( int node_id , int lf , int rg , int L , int R ) ;

    void _bfs_on_tree() ;

    int leaf_cnt ;
    seg_tree_node *node ;
} ;

#endif 