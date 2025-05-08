#include "src/async_dsa.hpp"
#include <vector> 
#include <pthread.h>
#include <sched.h> 
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex> 
#include <condition_variable>
#include <sys/mman.h>
#include <numa.h>
#include <numaif.h>
using namespace std ;
 
constexpr int thread_cnt = 16 , REPEAT = 5 ;
constexpr int str_per_thread = 100000 ;
constexpr double ns_to_s = 1.0 / 1000000000.0 ;
int bytes_min = 128 , bytes_max = 8192 , method = 0 ;
uint64_t POOL_SIZE_MAX = 1ULL * bytes_max * str_per_thread ;

class Barrier {
private:
    std::mutex mtx;
    std::condition_variable cv;
    uint64_t threshold;
    uint64_t count;
    uint64_t generation;
    
public:
    explicit Barrier(uint64_t count) : threshold(count), count(count), generation(0) {}
    
    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        uint64_t gen = generation; 
        if ( --count == 0) {
            // 最后一个到达的线程重置计数器
            generation++;
            count = threshold;
            cv.notify_all();
            return;
        } 
        // 等待当前代的屏障完成
        cv.wait(lock, [this, gen] { return gen != generation; });
    }
} barrier(thread_cnt) ;

struct shuffle_data{
    uint64_t len ;
    long long hashv ;
    char* bytes ;
    bool operator == ( const shuffle_data &another ) const {
        if( len != another.len ) return false ;
        uint64_t i = 0 ;
        for( ; i < len / 8 ; i += 8 )
            if( *((uint64_t*)(bytes+i)) != *((uint64_t*) another.bytes+i) ) return false ;
        for( ; i < len ; i ++ )
            if( bytes[i] != another.bytes[i] ) return false ;
        return true ;
    }
    #define c_offsetBasis 14695981039346656037ULL
    #define c_FNVPrime 1099511628211ULL 
    uint64_t FNV_hash( void* bs_, size_t len ) {
        uint64_t h  = c_offsetBasis;
        uint8_t *bs = (uint8_t*)bs_ ;
        for (size_t i = 0; i < len; ++i) {
            h = h ^ bs[i];
            h = h * c_FNVPrime;
        }
        return h;
    }
    #undef c_offsetBasis
    #undef c_FNVPrime

    long long gethash(){
        if( hashv != -1 ) return hashv ;
        hashv = bytes[0] < 0 ? -bytes[0] : bytes[0] ;
        // hashv = (long long)this->FNV_hash( bytes , len ) & 0x7fffffffffffffff ;
        return hashv ; 
    }
    void random_fill( Galois_LFSR &lsfr ){
        uint64_t i = 0 ;
        for( ; i < len / 8 ; i += 8 )   *((uint64_t*)(bytes + i)) = lsfr.lfsr64() ;
        for( ; i < len % 8 ; i ++ )     bytes[i] = lsfr.lfsr32() ;
    }
} ;
shuffle_data* before_shuffle[thread_cnt] ;
shuffle_data* after_shuffle[thread_cnt] ;
char* memory_pool_thr[thread_cnt] ;
char* before_pool_thr[thread_cnt] ;
std::atomic<uint64_t> memory_pool_used[thread_cnt] , str_cnt[thread_cnt] ;
void set_thread_affinity(std::thread& t, int core_id) ;
DSAbatch batch[thread_cnt] ;

void* alloc_binded_on_node( size_t len , int node = 0 ) {  
    struct bitmask *nodemask = numa_allocate_nodemask();
    numa_bitmask_setbit( nodemask , node ) ;
    // void *addr = malloc( len ) ;
    void* addr = mmap( NULL , len , PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS , -1 , 0 ) ;
    mbind( addr , len , MPOL_BIND , nodemask->maskp , nodemask->size , MPOL_MF_MOVE | MPOL_MF_STRICT );
    numa_bitmask_free( nodemask ) ;
    return addr ;
}
 
void prepare_data_thread( int tid ){
    Galois_LFSR lsfr ; 
    srand( time( 0 ) ) ;
    lsfr.set_LFSR_number( rand() ) ;
    srand( tid ) ;
    before_shuffle[tid] = (shuffle_data*)alloc_binded_on_node( sizeof(shuffle_data) * str_per_thread ); //new shuffle_data[str_per_thread] ;
    after_shuffle[tid] = (shuffle_data*)alloc_binded_on_node( sizeof(shuffle_data) * str_per_thread * 2 ); // new shuffle_data[str_per_thread*2] ;
    memory_pool_thr[tid] = (char*)alloc_binded_on_node(POOL_SIZE_MAX); // new char[POOL_SIZE_MAX] ;
    before_pool_thr[tid] = (char*)alloc_binded_on_node(POOL_SIZE_MAX); // new char[POOL_SIZE_MAX] ;
    for( long long i = 0 , lim = (long long)(POOL_SIZE_MAX); i < lim ; i += 4096 ) 
        memory_pool_thr[tid][i] = 1 ;
    memory_pool_thr[tid][POOL_SIZE_MAX - 1] = 1 ;
    memory_pool_used[tid].store( 0 ) ;
    str_cnt[tid].store( 0 ) ;
    char* now_ptr = memory_pool_thr[tid] ;
    for( int i = 0 ; i < str_per_thread ; i ++ ){
        uint64_t len = bytes_min + ( rand() % ( bytes_max - bytes_min ) ) ;
        before_shuffle[tid][i].len = len ;
        before_shuffle[tid][i].bytes = now_ptr ;
        before_shuffle[tid][i].hashv = -1 ;
        before_shuffle[tid][i].random_fill( lsfr ) ;
        before_shuffle[tid][i].gethash() ;
        now_ptr += len ;
    }
}
 
void shuffle_data_thread( int tid ){ 
    for( int i = 0 ; i < str_per_thread ; i ++ ){
        long long hashv = before_shuffle[tid][i].gethash() ;
        int aim = hashv % thread_cnt ;
        uint64_t len = before_shuffle[tid][i].len ; 
        uint64_t offset = memory_pool_used[aim].fetch_add( len ) ;
        uint64_t aim_i = str_cnt[aim].fetch_add( 1 ) ;  

        after_shuffle[aim][aim_i].len = len ;
        after_shuffle[aim][aim_i].bytes = memory_pool_thr[aim] + offset ;
        after_shuffle[aim][aim_i].hashv = hashv ; 
        *(after_shuffle[aim][aim_i].bytes + len - 1 ) = 1 ;
        if( method == 0 ) memcpy( after_shuffle[aim][aim_i].bytes , before_shuffle[tid][i].bytes , len ) ; 
        else batch[tid].submit_memcpy( after_shuffle[aim][aim_i].bytes , before_shuffle[tid][i].bytes , len ) ;
    } 
    batch[tid].wait() ;
    // batch[tid].print_stats() ;
}

bool check_shuffle_result_thread( int tid ){
    int this_cnt = str_cnt[tid].load() , error_cnt = 0 ; ;
    bool res = true ;
    for( int i = 0 ; i < this_cnt ; i ++ ){
        long long hashv = after_shuffle[tid][i].gethash() ; 
        if( (int)(hashv % thread_cnt) != tid ){
                printf( "result error @ thr %d, id = %d, √(%d) hashv = %lld, len = %ld\n" , tid , i , (int)(hashv % thread_cnt) , hashv , after_shuffle[tid][i].len ) ; 
            if( res ){
                res = false ;
            }
            error_cnt ++ ;
        }
    }
    if( !res ){
        printf( "thr %d : %d strings, %d errors\n" , tid , this_cnt , error_cnt ) ;
        fflush( stdout ) ;
    } else {
        // printf( "thr %d : %d strings, no errors\n" , tid , this_cnt ) ;
        // fflush( stdout ) ;
    }
    return res ;
}

void prepare_data_main(){
    uint64_t startns = timeStamp_hires() ;
    vector<std::thread> threads ;
    for( int i = 0 ; i < thread_cnt ; i ++ ){
        std::thread t( prepare_data_thread , i ) ;
        // set_thread_affinity( t , (i&1)*48 + i ) ;
        threads.push_back( std::move(t)) ;
    }
    for( auto& t : threads ) t.join();
    uint64_t endns = timeStamp_hires() ;
    
    long long tot_siz = 0 ;
    for( int i = 0 ; i < thread_cnt ; i ++ ){
        for( int j = 0 ; j < str_per_thread ; j ++ ){
            tot_siz += before_shuffle[i][j].len  ;
        }
    }
    tot_siz += sizeof( shuffle_data ) * str_per_thread * thread_cnt ; 
    double seconds = ( endns - startns ) * ns_to_s ;
    double speed = 1.0 * tot_siz / GB / seconds ;
    printf( "prepare_data() : %d threads, %d strings, time = %.3fs, bytes = %lld , speed = %.1fGB/s\n" , thread_cnt , str_per_thread , seconds , tot_siz , speed ) ;
    fflush( stdout ) ;
}

void shuffle_data_main(){
    double seconds = 0 ;
    for( int i = 0 , warm_up = 0 ; i < REPEAT ; i ++ ){
        uint64_t startns = timeStamp_hires() ;
        vector<std::thread> threads ;
        for( int i = 0 ; i < thread_cnt ; i ++ ){
            memory_pool_used[i].store( 0 ) ;
            str_cnt[i].store( 0 ) ;
        }
        for( int i = 0 ; i < thread_cnt ; i ++ ){
            std::thread t( shuffle_data_thread , i ) ;
            // set_thread_affinity( t , (i&1)*48 + i ) ;
            threads.push_back( std::move(t)) ;
        }
        for( auto& t : threads ) t.join();
        uint64_t endns = timeStamp_hires() ;
        if( warm_up == 0 ){
            warm_up ++ ; i -- ; continue ;
        }
        seconds += ( endns - startns ) * ns_to_s / REPEAT ;
    }

    long long tot_siz = 0 ;
    for( int i = 0 ; i < thread_cnt ; i ++ ){
        tot_siz += memory_pool_used[i].load() + sizeof( shuffle_data ) * str_cnt[i].load() ;
    } 
    double speed = 1.0 * tot_siz / GB / seconds ;
    if( method == 0 ){
        speed /= thread_cnt ;
        printf( "shuffle_data() : %d threads, %d strings, REPEAT = %d, time = %.3fs, bytes = %lld , speed = %.1fGB/s per CPU \n" , thread_cnt , str_per_thread , REPEAT , seconds , tot_siz , speed ) ;
    }
    else 
        printf( "shuffle_data() : %d threads, %d strings, REPEAT = %d, time = %.3fs, bytes = %lld , speed = %.1fGB/s \n" , thread_cnt , str_per_thread , REPEAT , seconds , tot_siz , speed ) ;
}

void check_shuffle_main(){ 
    vector<std::thread> threads ;
    for( int i = 0 ; i < thread_cnt ; i ++ ){
        std::thread t( check_shuffle_result_thread , i ) ;
        threads.push_back( std::move(t)) ;
    }
    for( auto& t : threads ) {
        t.join();
    }
}

void clear_main(){
    for( int i = 0 ; i < thread_cnt ; i ++ ){ 
        // free( before_shuffle[i] ) ;
        // free( after_shuffle[i] ) ;
        // free( memory_pool_thr[i] ) ;
        // free( before_pool_thr[i] ) ;
        munmap( before_shuffle[i] , sizeof(shuffle_data) * str_per_thread ) ;
        munmap( after_shuffle[i] , sizeof(shuffle_data) * str_per_thread * 2 ) ;
        munmap( memory_pool_thr[i] , POOL_SIZE_MAX ) ;
        munmap( before_pool_thr[i] , POOL_SIZE_MAX ) ; 
    } 
}
 
int main(){
    DSAagent::get_instance() ;
    for( int me = 0 ; me <= 0 ; me ++ ){
        method = me ;
        for( int L_length = 128 ; L_length <= 16384 ; L_length *= 2 ){  
            bytes_min = L_length ;
            bytes_max = L_length * 2 ;
            POOL_SIZE_MAX = 1ULL * bytes_max * str_per_thread ;
            printf( "method = %s, bytes_min = %d, bytes_max = %d\n" , method == 0 ? "CPU" : "DSA" , bytes_min , bytes_max ) ;
            prepare_data_main() ;
            shuffle_data_main() ;
            check_shuffle_main() ;
            clear_main() ;
        }
    }
}

void set_thread_affinity(std::thread& t, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset); 
    // 设置线程的亲和性
    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if( rc ){
        printf( "Error setting thread affinity: %d\n" , rc ) ;
        fflush( stdout ) ;
    }
}