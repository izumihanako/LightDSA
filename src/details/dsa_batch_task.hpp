#ifndef QWQ_DSA_BATCH_TASK_HPP
#define QWQ_DSA_BATCH_TASK_HPP
 
#include "dsa_agent.hpp"
#include "dsa_util.hpp"
#include "util.hpp" 
#include "dsa_conf.hpp"
#include "dsa_batch_redistribute.hpp"
#include <xmmintrin.h>  

class batch_record_queue{
public:
    int queue_cap ;
    int q_front , q_back , q_ecnt ;
    // back always point to a empty slot
    // front always point to something( or nothing if empty ) 
    int *queue ;
    batch_record_queue() = default ; 
    ~batch_record_queue() ;

    __always_inline void clear(){ q_front = 0 ; q_back = 0 ; q_ecnt = 0 ; } 
    __always_inline int front(){ return queue[q_front] ; }
    __always_inline int back(){ return queue[q_back] ; }
    __always_inline int empty(){ return q_ecnt == 0 ; }
    __always_inline int full(){ return q_ecnt == queue_cap ; }
    __always_inline int count(){ return q_ecnt ; }
    __always_inline int next( int x ){ return x + 1 == queue_cap ? 0 : x + 1 ; }
    void init( int cap ) ;
    void push_back( int x ) ;
    void pop() ;
    int pop_front() ;
    void delay_front_and_pop( int pos ) ;
    void print() ;
};

struct DSAbatch_task{
private : 
    // main desc
    dsa_hw_desc *bdesc ;
    // main comp
    dsa_completion_record *bcomp ;  
    
    // free queue and buzy queue
    batch_record_queue free_queue , buzy_queue ;

    // now writing batch
    int batch_idx , batch_base , desc_idx ;
    
    int batch_capacity , batch_siz , desc_capacity;
    // descs in batch
    dsa_hw_desc *descs ;
    // comps in batch
    dsa_completion_record *comps ;
    // last page fault address for descs
    char** last_fault_addr_rd ;
    char** last_fault_addr_wr ;
    
    // redistribute
    DSAtask_redistribute rdstrb ;

    void *wq_portal ;
    DSAworkingqueue *working_queue ;

public:
    int page_fault_resolving ;
    int batch_fail_cnt ;
    long long op_cnt ;
    long long op_bytes ;
 
public : 
    DSAbatch_task( int bsiz , int cap ) ;

    DSAbatch_task( int bsiz , int cap , DSAworkingqueue* wq ) ;

    ~DSAbatch_task() ;

private : 
    void init( int bsiz , int cap ) ;

    __always_inline int add_1_cap( int x ) { return x < 0 ? x + desc_capacity : x ; } 

    void prepare_desc( int idx , const dsa_rdstrb_entry &entry ) ;
    
    void alloc_descs() ;

    void free_descs() ;

    void add_no_op() ;

    void submit_forward() ;

    void collect_private() ;
    
    void PF_adjust_desc( int idx ) ;

    int resolve_error( int batch_idx ) ;

    void do_op( int batch_idx , int custom_desc_cnt ) ; 

    __always_inline bool is_pos_valid() { return batch_idx != -1 ; }

    __always_inline int now_pos(){ return desc_idx + batch_base ; }

    __always_inline void forward_pos(){ desc_idx ++ ; }

    __always_inline bool now_batch_full(){ return desc_idx == batch_siz ; }

public :  
    bool set_wq( DSAworkingqueue *new_wq ) ; 

    void add_op( dsa_opcode op_type ) ;

    void add_op( dsa_opcode op_type , void *dest , size_t len ) ;

    void add_op( dsa_opcode op_type , void *dest , const void* src , size_t len ) ;

    // check if all tasks has been done
    bool check() ;

    // wait until all tasks finish
    void wait() ;

    // free some finished tasks
    bool collect() ;

    // clear submit queue
    void clear() ;

    void print_stats() ;
    
    __always_inline bool empty() { return buzy_queue.empty() && desc_idx == 0 ; }
 
    __always_inline bool full() { return buzy_queue.count() == batch_capacity ; }

    __always_inline int count() { return buzy_queue.count() * batch_siz + desc_idx ; }  
} ; 

#endif