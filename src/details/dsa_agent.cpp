#include "dsa_agent.hpp"
#include <sys/mman.h>
#include <cstdio>

DSAworkingqueue::DSAworkingqueue( void *wq_portal , accfg_wq *wq ): wq_portal( wq_portal ) , wq( wq ) {
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        allocator = new DSAallocator();
    #endif
}

DSAworkingqueue::~DSAworkingqueue(){
    #ifdef ALLOCATOR_CONTIGUOUS_ENABLE
        delete allocator ;
    #endif 
    munmap( wq_portal , 0x1000 ) ;
}



/********************************************************************************/

DSAdevice::~DSAdevice(){
    for( auto wq : wq_list ){
        delete wq ;
    } 
}

DSAdevice::DSAdevice( accfg_device* devptr ): now_wq_id( 0 ) , wq_cnt( 0 ) , device( devptr ) { 
    wq_list.clear() ;
}


/********************************************************************************/

DSAagent::DSAagent(): now_dev_id( 0 ) , dev_cnt( 0 ) {
    accfg_new( &ctx ) ;
    init() ;
}

DSAagent::~DSAagent(){
    for( auto dev : devices ){
        delete dev ;
    }
    accfg_unref( ctx ) ;  
}

void DSAagent::init(){
    now_dev_id.store( 0 ) ;
    devices.clear() ;

    // find all available working queue
    accfg_device *device ;
    accfg_wq *wq ;
    void *wq_portal ;
    accfg_device_foreach( ctx , device ){ 
        DSAdevice* dev = new DSAdevice( device ) ; 
        if( accfg_device_get_type( device ) != ACCFG_DEVICE_DSA )
            continue ;
        accfg_wq_foreach( device , wq ) {
            int fd ;
            if( accfg_wq_get_type( wq ) != ACCFG_WQT_USER )
                continue ; 
            if( accfg_wq_get_state( wq ) != ACCFG_WQ_ENABLED )
                continue ;
            if( ( fd = open_wq( wq ) ) < 0 )
                continue ;
            close( fd ) ;
            wq_portal = wq_mmap( wq ) ;
            if( wq_portal == NULL ) continue ;
            DSAworkingqueue* dsa_wq = new DSAworkingqueue( wq_portal , wq ) ; 
            dev->wq_list.push_back( dsa_wq ); 
            dev->wq_cnt ++ ;  
        }
        if( dev->wq_cnt ) {
            devices.push_back( dev ) ;
            dev_cnt ++ ;
        } else {
            delete dev ;
        }
    }
    if( dev_cnt == 0 ){
        printf( "Could not find an available device, please check the DSA configuration.") ;
        exit( -1 ) ;
    }
    print_wqs() ; 
}

void DSAagent::print_wqs(){
    printf( "There are a total of %d device\n" , dev_cnt ) ;
    for( const auto &dev : devices ){
        printf( "device : %s\n" , dev->get_dev_name() ) ;
        for( const auto &x : dev->wq_list ){
            printf( "%s, %s, group %d\n" , x->get_name() ,
                    accfg_wq_get_mode( x->wq ) == ACCFG_WQ_DEDICATED ? "dedicated" : "shared" ,
                    accfg_wq_get_group_id( x->wq ) ) ;
        }
        accfg_engine *engine ;
        accfg_engine_foreach( dev->device , engine ){
            printf( "%s, group %d\n" , accfg_engine_get_devname( engine ) , 
                    accfg_engine_get_group_id( engine ) ) ;
        }
    } 
}

void *DSAagent::get_portal(){
    uint64_t tmp = now_dev_id.fetch_add( 1 ) ;
    return devices[tmp%dev_cnt]->get_portal() ;
}

void *DSAagent::get_portal( int dev_id ){ 
    return devices[dev_id]->get_portal() ;
}

void *DSAagent::get_portal( int dev_id , int wq_id ){ 
    return devices[dev_id]->wq_list[wq_id]->get_portal(); 
}

DSAdevice* DSAagent::get_device( int dev_id ) const {
    return devices[dev_id] ;
}
