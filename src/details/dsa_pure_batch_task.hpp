#ifndef QWQ_DSA_PURE_BATCH_TASK_HPP
#define QWQ_DSA_PURE_BATCH_TASK_HPP
 
#include "dsa_agent.hpp"
#include "dsa_util.hpp" 
#include "dsa_conf.hpp"
#include "dsa_batch_redistribute.hpp" 
#include <cstdio>
 
// this class is used only for experiments
// it is not used in the final version
struct DSA_pure_batch_task{
private : 
    // main desc
    dsa_hw_desc *bdesc ;
    // main comp
    dsa_completion_record *bcomp ;  
    
    int batch_siz , desc_cnt ;
    // descs in batch
    dsa_hw_desc *descs ;
    // comps in batch
    dsa_completion_record *comps ;  

    void *wq_portal ;
    DSAworkingqueue *working_queue ;

    bool is_doing_flag ;

public:
    int page_fault_resolving ;
    int batch_fail_cnt ;
    long long op_cnt ;
    long long op_bytes ;
 
public : 
    DSA_pure_batch_task( int bsiz ) ;

    DSA_pure_batch_task( int bsiz , DSAworkingqueue* wq ) ;

    ~DSA_pure_batch_task() ;

private : 
    void init( int bsiz ) ; 

    void prepare_desc( int idx , const dsa_rdstrb_entry &entry ) ;
    
    void alloc_descs() ;

    void free_descs() ; 
    
    void PF_adjust_desc( int idx ) ;

    void resolve_error() ;

    __always_inline int now_pos() { return desc_cnt ; }

    __always_inline void forward_pos(){ desc_cnt ++ ; }

    __always_inline bool is_pos_valid() { return desc_cnt < batch_siz ; }

public :  
    bool set_wq( DSAworkingqueue *new_wq ) ; 

    void add_op( dsa_opcode op_type ) ;

    void add_op( dsa_opcode op_type , void *dest , size_t len ) ;

    void add_op( dsa_opcode op_type , void *dest , const void* src , size_t len ) ;

    __always_inline void add_compare( const void *dest , const void* src , size_t len ) { 
        add_op( DSA_OPCODE_COMPARE , const_cast<void*>(dest) , src , len ) ; 
    }

    __always_inline void add_comp_pattern( const void *src , uint64_t pattern , size_t len ) {
        add_op( DSA_OPCODE_COMPVAL , (void*)(uintptr_t)pattern , const_cast<void*>(src) , len ) ; 
    }

    __always_inline void add_memfill( void *dest , uint64_t pattern , size_t len ) { 
        add_op( DSA_OPCODE_MEMFILL , (void*)(dest) , (void*)(uintptr_t)pattern , len ) ; 
    }

    __always_inline void add_memcpy( void *dest , const void* src , size_t len ) {  
        add_op( DSA_OPCODE_MEMMOVE , (void*)(dest) , (void*)(src) , len ) ; 
    }

    void do_op()  ;

    // check if all tasks has been done
    bool check( bool do_resolve_error ) ;

    // wait until all tasks finish
    void wait() ;

    void print_stats() ;

    __always_inline int get_status() { return op_status( bcomp->status ) ; }
    
    __always_inline bool empty() { return desc_cnt == 0 ; }
 
    __always_inline bool full() { return desc_cnt == batch_siz ; }

    __always_inline int count() { return desc_cnt ; }  

    __always_inline int is_doing() { return is_doing_flag ; }

    __always_inline void clear() { 
        if( is_doing_flag == false ) desc_cnt = 0 ;
        else printf( "DSA_pure_batch_task::clear() : task is doing\n" ) ;
    }
} ; 

#endif