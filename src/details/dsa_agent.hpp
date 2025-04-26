#ifndef QWQ_DSA_AGENT_HPP
#define QWQ_DSA_AGENT_HPP

#include "dsa_allocator.hpp"
#include "dsa_constant.hpp"
#include <atomic>
#include <vector>

struct DSAworkingqueue{ 
    void *wq_portal ; 
    accfg_wq *wq ;
    DSAallocator *allocator ;
    DSAworkingqueue(void *wq_portal , accfg_wq *wq ) ; 
    ~DSAworkingqueue() ;

    __always_inline void* get_portal() const { return wq_portal ; }
    __always_inline const char* get_name() const { return accfg_wq_get_devname( wq ) ; }
} ;



struct DSAdevice{ 
    int wq_cnt ;
    std::atomic<uint64_t> now_wq_id ;
    std::vector<DSAworkingqueue*> wq_list ; 
    accfg_device *device ;
 
    DSAdevice( accfg_device *devptr ) ;
    ~DSAdevice() ;
    __always_inline const char* get_dev_name() const {
        return accfg_device_get_devname( device ) ;
    }
    __always_inline DSAworkingqueue* get_wq() {
        uint64_t tmp = now_wq_id.fetch_add( 1 ) ;
        return wq_list[tmp%wq_cnt] ;
    }
} ;



class DSAagent{
private : 
    int dev_cnt ;
    std::atomic<uint64_t> now_dev_id ; 
    std::vector<DSAdevice*> devices ; 
    accfg_ctx *ctx ;
    static DSAagent inst ;

    void init() ;

    DSAagent() ;

    ~DSAagent() ;

public :
    void print_wqs() ; 

    __always_inline int get_dev_cnt(){ return devices.size() ; }
    
    __always_inline int get_wq_cnt( int dev_id ){ return devices[dev_id]->wq_list.size() ; }

    DSAworkingqueue *get_wq() ;

    DSAworkingqueue *get_wq( int dev_id ) ;

    DSAworkingqueue *get_wq( int dev_id , int wq_id ) ;

    DSAdevice* get_device( int dev_id ) const ;
 
    static DSAagent& get_instance(){
        static DSAagent inst ; // magic static
        return inst ;
    }

} ; 
 
#endif