#ifndef QWQ_DSA_TASK_HPP
#define QWQ_DSA_TASK_HPP

#include "util.hpp"
#include "dsa_agent.hpp"
#include "dsa_util.hpp"
#include <cstdio>

class DSAtask{
private : 
    dsa_hw_desc desc __attribute__( (aligned(64)) ) ;
    dsa_completion_record *comp ; 
    bool is_doing_flag ; 
    int op_cnt , page_fault_cnt , page_fault_resolving ;
    int retry_cnt , avg_fault_len ;
    char* last_fault_addr ;
    size_t op_bytes , ori_xfersize ;

    void *wq_portal ;
    DSAworkingqueue *working_queue ;

private :
    void PF_adjust_desc() ;

    void prepare_desc( dsa_opcode op_type ) ; 

    void prepare_desc( dsa_opcode op_type , void *src , uint32_t len , uint32_t stride = 0 ) ; 
    
    void prepare_desc( dsa_opcode op_type , const void *dest , const void* src , uint32_t len ) ;
    
    void init() ;

    void free_comp() ;

public : 
    DSAtask() ;

    DSAtask( DSAworkingqueue *wq ) ;

    ~DSAtask() ;

public :  
    void set_wq( DSAworkingqueue *wq ) ;

    __always_inline void set_op( dsa_opcode op_type ){ 
        prepare_desc( op_type ) ;     
    } 

    __always_inline void set_op( dsa_opcode op_type , void *src , uint32_t len , uint32_t stride = 0 ){ 
        prepare_desc( op_type , src , len , stride ) ;     
    } 

    __always_inline void set_op( dsa_opcode op_type , void *dest , const void* src , uint32_t len ){ 
        prepare_desc( op_type , dest , src , len ) ;     
    }

    void solve_pf() ;

    bool check() ;

    void do_op() noexcept( true ) ; 

    volatile uint8_t &comp_status_ref() ;

    volatile uint8_t *comp_status_ptr() ;

    __always_inline uint8_t get_comp_status() { return comp->status ; }

    __always_inline bool get_result(){ return comp->result ; }

    __always_inline size_t get_bytes_completed(){ return comp->bytes_completed ; }

    void print_stats() ;
} ; 

#endif