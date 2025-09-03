#include "src/lightdsa.hpp" 
#include "src/details/dsa_agent.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib> 
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
using namespace std ;

double ns_to_us = 0.001 ;
double us_to_s  = 0.001 * 0.001 ;
 
constexpr int REPEAT = 1 ;
int tdesc = 32768 , bsiz = 100000 ;

void test_dsa_batch( int cnt ){ 
    double st_time , ed_time , do_time ;
    double dsa_time = 0 , dsa_speed = 0 ;

    char *mem = (char*) aligned_alloc( 4096 , bsiz * 4096 ) ; 
    DSAtask** tasks = new DSAtask*[bsiz]; 
    for( int i = 0 ; i < bsiz ; i ++ )
        tasks[i] = new (mem + i * 4096) DSAtask( DSAagent::get_instance().get_wq() ) ;
    for( int tmp = 0 ; tmp < REPEAT ; tmp ++ ){  
        st_time = timeStamp_hires() ;  
        for( int i = 0 ; i < cnt ; i ++ ){
            int id = i % bsiz ;
            if( i >= bsiz ) while( !tasks[id]->check() ) ;
            tasks[id]->set_op( DSA_OPCODE_NOOP ) ;
            tasks[id]->do_op() ;
        }
        for( int i = 0 , lim = min( bsiz , cnt ); i < lim ; i ++ ) { while( !tasks[i]->check() ) ; }
        ed_time = timeStamp_hires() , do_time = ed_time - st_time , st_time = ed_time ; 
        dsa_time += ( do_time ) / REPEAT ;
    } 
    delete[] tasks ;

    dsa_time *= ns_to_us ; 
    dsa_speed = cnt / dsa_time ; 
    printf( "%d desc NO OP \n" , cnt ) ; 
    printf( "    ; DSA_single cost %5.2lfus, speed %.2fdesc/us\n" , dsa_time , dsa_speed ) ; 
    puts( "" ) ;
}

DSAop ___ ;
int main( int argc , char *argv[] ){
    if( argc > 1 ){ 
        tdesc = atoll( argv[1] ) ;
        if( argc > 2 ) bsiz = atoi(argv[2]) ;
    } else {
        printf( "Usage     : %s tdesc\n" , argv[0] ) ;
        printf( "op_cnt    : num of noop operation\n" ) ;
        printf( "desc_cnt  : num of descriptor\n" ) ;
        return 0 ;
    }

    printf( "DSA noop cnt %d, desc_cnt = %d\n" , tdesc , bsiz ) ; fflush(stdout) ; 
    test_dsa_batch( tdesc ) ; 
}

//