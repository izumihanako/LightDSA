#ifndef QWQ_DSA_TASK_HPP
#define QWQ_DSA_TASK_HPP

#include "dsa_util.hpp"
#include "util.hpp"
#include <cstdio>

class DSAtask{
private : 
    dsa_hw_desc desc  __attribute__( (aligned(64)) ) ;
    dsa_completion_record __attribute__( (aligned(32)) ) comp ; 
    void *wq_portal ;
    bool is_doing_flag ; 

private :
    void PF_adjust_desc() ;

    void prepare_desc( dsa_opcode op_type ) ; 

    void prepare_desc( dsa_opcode op_type , void *src , uint32_t len , uint32_t stride = 0 ) ; 
    
    void prepare_desc( dsa_opcode op_type , const void *dest , const void* src , uint32_t len ) ;
    
    void clear() ;

public : 
    DSAtask() ;

    DSAtask( void* portal_ ) ;

public :  
    void set_port( void* wq_portal_ ) ;

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

    uint64_t comp_fault_addr() ;

    volatile uint8_t &comp_status_ref() ;

    volatile uint8_t *comp_status_ptr() ;
} ; 

#endif