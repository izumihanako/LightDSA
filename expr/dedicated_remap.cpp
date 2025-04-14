#include "src/details/dsa_agent.hpp"
#include "src/async_dsa.hpp"


int main(){
    DSAagent &dsa = DSAagent::get_instance() ;
    DSAdevice *device0 = dsa.get_device( 0 ) ;
    accfg_wq *wq ;
    accfg_wq_foreach( device0->device , wq ){
        if( accfg_wq_get_type( wq ) != ACCFG_WQT_USER )
            continue ;
        if( accfg_wq_get_state( wq ) != ACCFG_WQ_ENABLED )
            continue ;
        printf( "wq name = %s\n" , accfg_wq_get_devname( wq ) ) ;
        while( 1 ) ;
        if( accfg_wq_get_mode( wq ) == ACCFG_WQ_DEDICATED ){
            printf( "fd0 = %p\n" , device0->wq_list[0]->wq_portal ) ;
            void *fd1 = wq_mmap( wq ) ;
            void *fd2 = wq_mmap( wq ) ;
            printf( "fd1 = %p, fd2 = %p\n" , fd1 , fd2 ) ;
        } 
    }
}