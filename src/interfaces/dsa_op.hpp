#ifndef QWQ_DSA_OP_HPP
#define QWQ_DSA_OP_HPP
 
#include "details/dsa_task.hpp"

class DSAop{
private :
    DSAtask dtask ;
    int to_dsa , to_cpu ;
    size_t align_sum ;
    char paddings[ 64 - (sizeof(DSAtask)+sizeof(int)*2+sizeof(size_t))%64 ] ;

public :
    DSAop() ;

    bool async_memcpy( void *dest , const void* src , size_t len ) noexcept( true ) ;
    bool async_memfill( void *dest , uint64_t pattern , size_t len ) noexcept( true ) ;
    bool async_flush( void *dest , size_t len ) noexcept( true ) ;
    bool async_comp_pattern( const void *src , uint64_t pattern , size_t len ) noexcept( true ) ;
    bool async_compare( const void *dest , const void* src , size_t len ) noexcept( true ) ;
    
    void sync_memcpy( void *dest , const void* src , size_t len ) noexcept( true ) ;
    void sync_memfill( void *dest , uint64_t pattern , size_t len ) noexcept( true ) ;
    void sync_flush( void *dest , size_t len ) noexcept( true ) ;
    void sync_comp_pattern( const void *src , uint64_t pattern , size_t len ) noexcept( true ) ;
    void sync_compare( const void *dest , const void* src , size_t len ) noexcept( true ) ;

    bool check() ; 
    void wait() ; 
    void print_stats() ;
    bool compare_res() ;
    size_t compare_differ_offset() ;
} ;

#endif 