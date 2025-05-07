#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h> 
#include <numa.h> 
#include <numaif.h>
#include <string>
#include <cmath>
#include "src/async_dsa.hpp"

using namespace std ;

int REPEAT = 5 ;
struct Migration_task{
    void *src ;
    void *dest ;
    size_t len ; 
    Migration_task( void* off_src_ , void* off_dest_ , size_t len_ ) : src( off_src_ ) , dest( off_dest_ ) , len( len_ ) {} 
} ;

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
    // 方法2: 使用get_mempolicy系统调用 (更精确)
    if (get_mempolicy(&node, NULL, 0, addr, MPOL_F_NODE | MPOL_F_ADDR) != 0) {
        perror("get_mempolicy failed");
        return -1;
    } 
    return node;
}

void* alloc_page_on_node( bool huge , int node ) { 
    int flags = MAP_PRIVATE | MAP_ANONYMOUS ;
    size_t len = huge ? 2 * MB : 4 * KB ;
    if( huge ) flags |= MAP_HUGETLB ; 
    if (numa_available() == -1) {
        printf( "NUMA not available\n" ); 
        return nullptr;
    }  
    struct bitmask *nodemask = numa_allocate_nodemask();
    numa_bitmask_setbit( nodemask , node );
    numa_bind( nodemask );
     
    void* ptr = mmap(nullptr, len , PROT_READ | PROT_WRITE, flags, -1, 0); 
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return nullptr;
    }  
    mbind( ptr , len , MPOL_BIND , nodemask->maskp , nodemask->size , MPOL_MF_MOVE | MPOL_MF_STRICT );
    // mlock( ptr , len ) ; // this will fill page table
    numa_bitmask_free( nodemask );
    numa_set_localalloc(); 
    return ptr;
}
 
// 每个 migration 都从 numa0 迁移到 numa1 
void genMigration( size_t migration_cnt , double Huge_ratio , vector<Migration_task> &tasks ){
    int Huge_cnt = 0 ;
    int Small_cnt = 0 ;
    srand( 0 ) ;
    size_t totlen = 0 ;
    tasks.clear() ;
    uint64_t tot_time = 0 ;
    for( size_t i = 0 ; i < migration_cnt ; i ++ ){
        size_t len ;
        uint64_t start_time = timeStamp_hires() ;
        if( 1.0 * rand() / RAND_MAX < Huge_ratio ){
            Huge_cnt ++ ;
            len = 2 * MB ;
        } else {
            Small_cnt ++ ;
            len = 4 * KB ;
        }
        totlen += len * 2 ;
        void *src = (void*) alloc_page_on_node( len == 2 * MB , 0 ) ;
        void *dest = (void*) alloc_page_on_node( len == 2 * MB , 1 ) ;
        if( src == nullptr || dest == nullptr ) {
            printf( "alloc page failed" ) ; 
            printf( "Huge_cnt = %d , Small_cnt = %d\n" , Huge_cnt , Small_cnt ) ;
            getchar() ;
        }  
        uint64_t end_time = timeStamp_hires() ;
        tot_time += end_time - start_time ;
        for( size_t j = 0 ; j < len ; j += 1 ) {
            *((char*)src + j) = 'A' ; 
            *((char*)dest + j) = 'B' ;
        }  
        tasks.push_back( Migration_task( src , dest , len ) ) ; 
    }
    random_shuffle( tasks.begin() , tasks.end() ) ;
    printf( "Huge_cnt = %d , Small_cnt = %d , alloc speed = %.2fGB/s\n" , Huge_cnt , Small_cnt , 
            1.0 * totlen / tot_time * 1000000000 / GB ) ;
}

void free_tasks( vector<Migration_task> &tasks ){
    for( size_t i = 0 ; i < tasks.size() ; i ++ ){
        munmap( tasks[i].src , tasks[i].len ) ;
        munmap( tasks[i].dest , tasks[i].len ) ;
    }
    tasks.clear() ;
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

size_t huge_split_granu = 128 * KB , cached_splited_max = 20 ;
vector<Migration_task> tasks ;
void test_migration( size_t migration_cnt , double Huge_ratio , int method ){ 
    uint64_t tot_siz = 0 ;
    DSAbatch dsa_batch( 32 , 200 ) ;
    double tot_time = 0 ;
    for( int repeat = 0 , warmup = 0 ; repeat < REPEAT ; repeat ++ ){ 
        tot_siz = 0 ;
        genMigration( migration_cnt , Huge_ratio , tasks ) ;
        for( size_t i = 0 ; i < tasks.size() ; i ++ ){
            // if( get_numa_node( tasks[i].src ) != 0 || get_numa_node( tasks[i].dest ) != 1 ){
            //     printf( "migration task %lu src @ %p dest @ %p\n" , i , tasks[i].src , tasks[i].dest ) ;
            //     printf( "migration task %lu src node %d dest node %d\n" , i , get_numa_node( tasks[i].src ) , get_numa_node( tasks[i].dest ) ) ;
            //     getchar() ;
            // }
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
                dsa_batch.submit_memcpy( tasks[i].dest , tasks[i].src , tasks[i].len ) ;
            }
            dsa_batch.wait() ;
            ed_time = timeStamp_hires() ;
        }
        check_correct( tasks ) ;
        free_tasks( tasks ) ;
        if( warmup == 0 ){ warmup ++ ; repeat -- ; continue ; }
        double time_cost = ( ed_time - st_time ) / 1000000000.0 ;
        tot_time += time_cost / REPEAT ;
        printf( "  repeat %d, time cost %.3fs\n" , repeat , time_cost ) ;
    } 
    double speed = 1.0 * tot_siz / GB / tot_time ;
    printf( "method = %-9s | migration_cnt = %lu | Huge_ratio = %.2f%% | tot_siz = %8s | time cost %.3fs | speed = %.3fGB/s\n" , 
            method == 0 ? "memcpy" : ( method == 1 ? "naive dsa" : "our dsa" ) , 
            migration_cnt , Huge_ratio * 100 , stdsiz( tot_siz ).c_str() , tot_time , speed ) ;
    if( method == 1 ) dsa_batch.print_stats() ;
}

DSAop ___ ;
int main(){ 
    // int x = 0 ;
    // uint64_t st_time = timeStamp_hires() ;
    // for( int i = 0 ; i <= 8888 ; i ++ ){
    //     // char* tmp0 = (char*) malloc( 4 * KB ) ;
    //     char* tmp0 = (char*) mmap( nullptr , 2 * MB , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB , -1 , 0 ) ;
    //     // char* tmp0 = (char*) alloc_page_on_node( false , 0 ) ;
    //     // char* tmp0 = (char*) numa_alloc_onnode( 2 * MB , 0 ) ;
    //     if( get_numa_node( tmp0 ) != 0 ){
    //         printf( "alloc huge page on node %d failed\n" , get_numa_node( tmp0 ) ) ;
    //         getchar() ;
    //     }
    //     tmp0[0] = rand() ;
    //     x ^= *tmp0 ;
    // }
    // uint64_t ed_time = timeStamp_hires() ;
    // printf( "speed = %.2fGB/s\n" , 1.0 * 2 * MB * 8888 / ( ed_time - st_time ) * 1000000000 / GB ) ;
    // return 0 ;
    // vector<Migration_task> tasks ;
    // genMigration( 10000 , 0 , tasks ) ;
    // return 0 ;

    char* tmp1 = (char*) alloc_page_on_node( true , 0 ) ;
    char* tmp2 = (char*) alloc_page_on_node( true , 1 ) ;
    printf( "tmp1 @ %p on node %d , tmp2 @ %p on node %d\n" , tmp1 , get_numa_node( tmp1 ) , tmp2 , get_numa_node( tmp2 ) ) ;
    // printf( "tmp1[0] = %d , tmp2[0] = %d\n" , *tmp1 , *tmp2 ) ;
    size_t migration_cnt = 10000 ;
    double Huge_ratio = 0.1 ;
    for( int me = 0 ; me <= 1 ; me ++ ){
        test_migration( migration_cnt , Huge_ratio , me ) ;
    }
}