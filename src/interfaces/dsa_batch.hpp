#ifndef QWQ_DSA_BATCH_HPP
#define QWQ_DSA_BATCH_HPP

#include "details/dsa_batch_task.hpp" 
#include "details/dsa_conf.hpp"

struct DSAbatch{ 
public :
    DSAbatch_task db_task ;

    DSAbatch( int bsiz = DEFAULT_BATCH_SIZE , int cap = DEFAULT_BATCH_CAPACITY  ) ;

    bool submit_compare( const void *dest , const void* src , size_t len ) noexcept( true ) ;

    bool submit_comp_pattern( const void *src , uint64_t pattern , size_t len ) noexcept( true ) ;

    bool submit_memfill( void *dest , uint64_t pattern , size_t len ) noexcept( true ) ;

    bool submit_memmove( void *dest , const void* src , size_t len ) noexcept( true ) ;

    bool submit_flush( void *dest , size_t len ) noexcept( true ) ;

    bool submit_noop() noexcept( true ) ;
     
    bool check() ;

    void wait() ;

    void print_stats() ; 

    int to_dsa ;
    int to_cpu ;
    size_t align_sum ; 
} ;

#endif 