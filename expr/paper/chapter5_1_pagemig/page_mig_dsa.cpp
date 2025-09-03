#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h> 
#include <numa.h> 
#include <numaif.h>
#include <string>
#include <cmath>
#include <queue>
#include "src/lightdsa.hpp" 

using namespace std ;

int REPEAT = 5 ;
struct Migration_task{
    void *src ;
    void *dest ;
    size_t len ; 
    Migration_task( void* off_src_ , void* off_dest_ , size_t len_ ) : src( off_src_ ) , dest( off_dest_ ) , len( len_ ) {} 
} ;
 
constexpr size_t pool_size = 16 * GB ;
char *huge_pool_0 , *huge_pool_1 ;
char* small_pool_0 , *small_pool_1 ;

string stdsiz( size_t siz ) {
    double regsiz = siz ;
    int regcnt = 0 ;  
    while( regsiz >= 1024 ) regsiz /= 1024 , regcnt ++ ;
    const char regstr[][3] = { "B\0" , "KB" , "MB" , "GB" } ; 
    char rt[20] ;
    regsiz = floor( regsiz * 1000 + 0.5 ) / 1000 ;
    sprintf( rt , "%.4g%2s" , regsiz , regstr[regcnt] ) ;
    return string( rt ) ;
}

int get_numa_node(void* addr) {
    int node = -1;  
    if (get_mempolicy(&node, NULL, 0, addr, MPOL_F_NODE | MPOL_F_ADDR) != 0) {
        perror("get_mempolicy failed");
        return -1;
    } 
    return node;
}

void* alloc_mem_on_node( size_t len , int node , int huge ) { 
    int flags = MAP_PRIVATE | MAP_ANONYMOUS ; 
    if( huge ) flags |= MAP_HUGETLB ; 
    if (numa_available() == -1) {
        printf( "NUMA not available\n" ); 
        return nullptr;
    }  
    struct bitmask *nodemask = numa_allocate_nodemask();
    numa_bitmask_setbit( nodemask , node );
    void* ptr = mmap(nullptr, len , PROT_READ | PROT_WRITE, flags, -1, 0); 
    if (ptr == MAP_FAILED) {
        perror("mmap"); 
        return nullptr;
    }
    mbind( ptr , len , MPOL_BIND , nodemask->maskp , nodemask->size , MPOL_MF_MOVE | MPOL_MF_STRICT );
    numa_bitmask_free( nodemask );
    for( size_t i = 0 ; i < len ; i += 4096 ) {
        *((char*)ptr + i) = 'A' ; 
    }
    return ptr;
}
 
// 每个 migration 都从 numa0 迁移到 numa1 
void genMigration( size_t migration_cnt , double Huge_ratio , vector<Migration_task> &tasks ){
    int Huge_cnt = 0 ;
    int Small_cnt = 0 ;
    srand( 0 ) ; 
    tasks.clear() ; 
    for( size_t i = 0 ; i < migration_cnt ; i ++ ){  
        if( 1.0 * rand() / RAND_MAX < Huge_ratio ){
            Huge_cnt ++ ;
            size_t len = 2 * MB ;
            void* src = huge_pool_0 + ( rand() % ( pool_size / len ) ) * len ;
            void* dest = huge_pool_1 + ( rand() % ( pool_size / len ) ) * len ;
            tasks.push_back( Migration_task( src , dest , len ) ) ; 
        } else {
            Small_cnt ++ ;
            size_t len = 4 * KB ;
            void* src = small_pool_0 + ( rand() % ( pool_size / len ) ) * len ;
            void* dest = small_pool_1 + ( rand() % ( pool_size / len ) ) * len ;
            tasks.push_back( Migration_task( src , dest , len ) ) ; 
        }  
    }
    random_shuffle( tasks.begin() , tasks.end() ) ; 
    printf( "Huge_cnt = %d , Small_cnt = %d\n" , Huge_cnt , Small_cnt ) ;
} 

void check_correct( vector<Migration_task> &tasks ){
    for( size_t i = 0 ; i < tasks.size() ; i ++ ){
        char *src = (char*) tasks[i].src ;
        char *dest = (char*) tasks[i].dest ;
        size_t len = tasks[i].len ;
        for( size_t j = 0 ; j < len ; j ++ ){
            if( src[j] != dest[j] ){
                printf( "migration task %lu src @ %p dest @ %p\n" , i , src , dest ) ;
                printf( "migration task %lu src[%lu] = %c dest[%lu] = %c\n" , i , j , src[j] , j , dest[j] ) ;
                getchar() ;
            }
        }
    }
}
 
vector<Migration_task> tasks ;
void test_migration( size_t migration_cnt , double Huge_ratio , int method ){ 
    uint64_t tot_siz = 0 ;
    DSAbatch dsa_batch( 32 , method == 1 ? 20 : 80 ) ;
    double tot_time = 0 ;
    genMigration( migration_cnt , Huge_ratio , tasks ) ;
    for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){ 
        tot_siz = 0 ;
        for( size_t i = 0 ; i < tasks.size() ; i ++ ){
            if( get_numa_node( tasks[i].src ) != 0 || get_numa_node( tasks[i].dest ) != 1 ){
                printf( "migration task %lu src @ %p dest @ %p\n" , i , tasks[i].src , tasks[i].dest ) ;
                printf( "migration task %lu src node %d dest node %d\n" , i , get_numa_node( tasks[i].src ) , get_numa_node( tasks[i].dest ) ) ;
                getchar() ;
            }
            tot_siz += tasks[i].len ;
        }
        uint64_t st_time = 0 , ed_time = 0 ;
        if( method == 0 ){
            st_time = timeStamp_hires() ;
            for( size_t i = 0 ; i < tasks.size() ; i ++ )
                memcpy( tasks[i].dest , tasks[i].src , tasks[i].len ) ;
            ed_time = timeStamp_hires() ;
        } else if( method == 1 ){ // naive DSA
            st_time = timeStamp_hires() ;
            for( size_t i = 0 ; i < tasks.size() ; i ++ ){
                dsa_batch.submit_memmove( tasks[i].dest , tasks[i].src , tasks[i].len ) ;
            }
            dsa_batch.wait() ;
            ed_time = timeStamp_hires() ;
        } else if( method == 2 ){ // naive queue
            vector<Migration_task> tasks_splits ;
            tasks_splits.reserve( migration_cnt * Huge_ratio * 21 ) ;
            st_time = timeStamp_hires() ;
            size_t now_split_id = 0 ;
            for( size_t i = 0 , small_submit = 0 ; i < tasks.size() ; i ++ ){
                if( tasks[i].len == 2 * MB ){
                    for( int j = 0 ; j < 10 ; j ++ ){
                        size_t len = tasks[i].len / 10 ;
                        void* src = (char*)tasks[i].src + j * len ;
                        void* dest = (char*)tasks[i].dest + j * len ;
                        tasks_splits.push_back( Migration_task( src , dest , len ) ) ;
                    }
                } else { 
                    small_submit ++ ;
                    if( small_submit % 16 == 0 && tasks_splits.size() > now_split_id ){
                        Migration_task &task = tasks_splits[now_split_id] ;
                        dsa_batch.submit_memmove( task.dest , task.src , task.len ) ;
                        now_split_id ++ ;
                    }
                    dsa_batch.submit_memmove( tasks[i].dest , tasks[i].src , tasks[i].len ) ;
                }
            }
            while( tasks_splits.size() > now_split_id ){
                Migration_task &task = tasks_splits[now_split_id] ;
                dsa_batch.submit_memmove( task.dest , task.src , task.len ) ;
                now_split_id ++ ;
            }
            dsa_batch.wait() ;
            ed_time = timeStamp_hires() ;
        }
        check_correct( tasks ) ;
        if( warmup == 0 ){ warmup ++ ; repeat -- ; continue ; }
        double time_cost = ( ed_time - st_time ) / 1000000000.0 ;
        tot_time += time_cost / REPEAT ;
        printf( "  repeat %d, time cost %.3fs\n" , repeat , time_cost ) ;
    } 
    double speed = 1.0 * tot_siz / GB / tot_time ;
    printf( "method = %-9s | migration_cnt = %lu | Huge_ratio = %.2f%% | tot_siz = %8s | time cost %.3fs | speed = %.3fGB/s\n" , 
            method == 0 ? "memcpy" : ( method == 1 ? "naive dsa" : "our dsa" ) , 
            migration_cnt , Huge_ratio * 100 , stdsiz( tot_siz ).c_str() , tot_time , speed ) ; 
}

DSAop ___ ;
int main(){ 
    huge_pool_0 = (char*)alloc_mem_on_node( pool_size , 0 , 1 ) ;
    huge_pool_1 = (char*)alloc_mem_on_node( pool_size , 1 , 1 ) ;
    small_pool_0 = (char*)alloc_mem_on_node( pool_size , 0 , 0 ) ;
    small_pool_1 = (char*)alloc_mem_on_node( pool_size , 1 , 0 ) ;
    // char* tmp1 = (char*) alloc_page_on_node( true , 0 ) ;
    // char* tmp2 = (char*) alloc_page_on_node( true , 1 ) ;
    // printf( "tmp1 @ %p on node %d , tmp2 @ %p on node %d\n" , tmp1 , get_numa_node( tmp1 ) , tmp2 , get_numa_node( tmp2 ) ) ;
    // printf( "tmp1[0] = %d , tmp2[0] = %d\n" , *tmp1 , *tmp2 ) ;
    size_t migration_cnt = 100000 ; 
    for( double ratio = 0.00001 ; ratio <= 0.004 ; ratio *= 2 ){
        for( int me = 1 ; me <= 1 ; me ++ ){
            test_migration( migration_cnt , ratio , me ) ;
        }
    } 
}