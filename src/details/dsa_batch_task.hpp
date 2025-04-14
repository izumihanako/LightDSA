#ifndef QWQ_DSA_BATCH_TASK_HPP
#define QWQ_DSA_BATCH_TASK_HPP
 
#include "dsa_agent.hpp"
#include "dsa_util.hpp"
#include "util.hpp"
#ifndef FLAG_CACHE_CONTROL
#include <xmmintrin.h> 
#endif

struct DSAbatch_task{
// private : 
    // main desc
    dsa_hw_desc *bdesc ;
    // main comp
    dsa_completion_record *bcomp ;
    
    // queue info
    int q_front , q_submit , q_back , q_ecnt , desc_capacity ;
    int batch_capacity , batch_siz ;
    // descs in batch
    dsa_hw_desc *descs ;
    // comps in batch
    dsa_completion_record *comps , *front_comp ;
    // volatile uint8_t *front_status ;

    void *wq_portal ;
    DSAworkingqueue *working_queue ;
 
public : 
    DSAbatch_task( int bsiz , int cap ) ;

    DSAbatch_task( int bsiz , int cap , DSAworkingqueue* wq ) ;

    ~DSAbatch_task() ;

// private : 
    __always_inline int add_1_cap( int x ) { return x < 0 ? x + desc_capacity : x ; } 

    void prepare_desc( dsa_opcode op_type ) ;

    void prepare_desc( dsa_opcode op_type , void *src , uint32_t len , uint32_t stride = 0 ) ;
    
    void prepare_desc( dsa_opcode op_type , void *dest , const void* src , uint32_t len ) ;

    void alloc_descs( int cap ) ;

    void free_descs() ;

    void add_no_op() ;

    void submit_forward() ;

    void collect_private() ;
    
    void PF_adjust_desc( int desc_idx ) ;

    void check_front() ;

    void do_op( int desc_idx )  ;

    void solve_pf( int desc_idx ) ; 

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

    dsa_completion_record* get_front_comp() ;
    
    __always_inline bool empty() { return q_ecnt == 0 ; }

    __always_inline bool full() { return q_ecnt == desc_capacity ; }

    __always_inline int count() { return q_ecnt ; }  
} ; 

#endif