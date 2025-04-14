#ifndef QWQ_DSA_MEMCPY_HPP
#define QWQ_DSA_MEMCPY_HPP
 
#include "details/dsa_task.hpp"

class DSAmemcpy{
private :
    DSAtask dtask ;

public :
    DSAmemcpy() ;
    
    void do_async( void *dest , const void* src , size_t len ) noexcept( true ) ;

    bool check() ;

    void wait() ;

    void* do_sync( void* dest , const void* src , size_t len ) noexcept( true ) ;

    void* operator() ( void* dest , const void* src , size_t len ) noexcept( true ) ;
} ;

#endif 