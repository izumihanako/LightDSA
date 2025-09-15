#include "dsa_agent.hpp"
#include "util.hpp"
#include <sys/mman.h>
#include <cstdio>
#include <mutex>

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

DSAdevice::DSAdevice( accfg_device* devptr ): wq_cnt( 0 ) , now_wq_id( 0 ) , device( devptr ) { 
    wq_list.clear() ;
}


/********************************************************************************/

DSAagent::DSAagent(): dev_cnt( 0 ) , now_dev_id( 0 ) {
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

    printf_RGB( 0x88B806 , "\nDSA configures:\n" ) ;
    #if defined(DESCS_OUT_OF_ORDER_RECYCLE_ENABLE)
        printf_RGB( 0x00cc33 , "+  OoO recycle enable, T_init = %d\n" , OUT_OF_ORDER_RECYCLE_T_INIT ) ;
    #else 
        printf_RGB( 0xcc0011 , "-  OoO recycle disable\n" ) ;
    #endif

    #if defined(DESCS_INBATCH_DESCRIPTORS_MIXING_ENABLE)
        printf_RGB( 0x00cc33 , "+  In-batch descs mixing enable\n" ) ;  
    #else 
        printf_RGB( 0xcc0011 , "-  In-batch descs mixing disable\n" ) ;
    #endif

    #if defined(DESCS_ADDRESS_ALIGNMENT)
        printf_RGB( 0x00cc33 , "+  Write address 64Byte alignment enable " ) ;
    #else 
        printf_RGB( 0xcc0011 , "-  Write address 64Byte alignment disable " ) ; 
    #endif 
    if( _FLAG_CC_ ) {
        printf_RGB( 0x00cc33 , "+ with cache control\n" ) ;
    } else {
        printf_RGB( 0xcc0011 , "- without cache control\n" ) ;
    } 

    #if defined(ALLOCATOR_CONTIGUOUS_ENABLE)
        printf_RGB( 0x00cc33 , "+  Contiguous allocator enable" ) ;
        #if defined(ALLOCATOR_USE_HUGEPAGE)
            printf_RGB( 0x00cc33 , "(enable huge-page)" ) ;
        #endif
        puts( "" ) ;
    #else
        printf_RGB( 0xcc0011 , "-  Contiguous allocator disable\n" ) ;
    #endif

    #if defined(INTERLEAVED_PAGEFAULT_ENABLE)
        printf_RGB( 0x00cc33 , "+  Interleaved page fault enable( pf_limit = %d Bytes )\n" , 
                DSA_PF_LEN_LIMIT ) ;
    #else
        printf_RGB( 0xcc0011 , "-  Interleaved page fault disable\n" ) ;
    #endif
     
    printf_RGB( 0x88B806 , "FLAGS: ") ;
    if( _FLAG_BOF_ ) printf_RGB( 0x00cc33 , "+BOF " ) ; else printf_RGB( 0xcc0011 , "-BOF " ) ;
    if( _FLAG_CC_ )  printf_RGB( 0x00cc33 , "+CC " ) ; else printf_RGB( 0xcc0011 , "-CC " ) ;
    if( _FLAG_DRDBK_ ) printf_RGB( 0x00cc33 , "+DRDBK " ) ; else printf_RGB( 0xcc0011 , "-DRDBK " ) ;
    #if defined( SHORT_TO_CPU )
        printf_RGB( 0x00cc33 , "+SHORT_TO_CPU " ) ;
    #else
        printf_RGB( 0xcc0011 , "-SHORT_TO_CPU " ) ;
    #endif
    if( IS_CPU_FLUSH ) printf_RGB( 0x00cc33 , "+CPU_FLUSH " ) ; else printf_RGB( 0xcc0011 , "-CPU_FLUSH " ) ;
    puts( "" ) ; 
    fflush( stdout ) ;
}

void DSAagent::print_wqs(){
    printf_RGB( 0x88B806 , "There are a total of %d device\n" , dev_cnt ) ;
    for( const auto &dev : devices ){
        printf_RGB( 0x88B806 , "device : %s\n" , dev->get_dev_name() ) ;
        for( const auto &x : dev->wq_list ){
            printf_RGB( 0x1450B8 , "·>  %s, %s, group %d\n" , x->get_name() ,
                    accfg_wq_get_mode( x->wq ) == ACCFG_WQ_DEDICATED ? "dedicated" : "shared" ,
                    accfg_wq_get_group_id( x->wq ) ) ;
        }
        accfg_engine *engine ;
        accfg_engine_foreach( dev->device , engine ){
            if( accfg_engine_get_group_id( engine ) == -1 )
                continue ;
            printf_RGB( 0xB8B104 , "·>  %s, group %d\n" , accfg_engine_get_devname( engine ) , 
                    accfg_engine_get_group_id( engine ) ) ;
        }
    } 
}

DSAworkingqueue *DSAagent::get_wq(){
    uint64_t tmp = now_dev_id.fetch_add( 1 ) ;
    return devices[tmp%dev_cnt]->get_wq() ;
}

DSAworkingqueue *DSAagent::get_wq( int dev_id ){ 
    return devices[dev_id]->get_wq() ;
}

DSAworkingqueue *DSAagent::get_wq( int dev_id , int wq_id ){ 
    return devices[dev_id]->wq_list[wq_id] ; 
}

DSAdevice* DSAagent::get_device( int dev_id ) const {
    return devices[dev_id] ;
}

DSAagent* DSAagent::inst = nullptr ;
std::once_flag DSAagent::init_flag ;