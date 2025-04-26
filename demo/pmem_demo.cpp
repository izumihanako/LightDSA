#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/async_dsa.hpp"

int use_dsa = 1 ;
DSAbatch batch ;

typedef struct {
    size_t file_size;     // 当前文件总大小
    size_t used_size;     // 已使用的空间大小
    size_t extend_size;   // 每次扩展的大小 (例如 4MB)
    const char* file_path;     // 文件路径
    char *pmem_addr;      // 映射的持久内存地址
    int is_pmem;          // 是否为真正的持久内存
} PMFileHandler;

int check_if_path_is_pmem(const char *path) {
    int is_pmem;
    size_t mapped_len; 
    struct stat st;
    if (stat(path, &st) == -1) { // check if file exists
        void *addr = pmem_map_file(path, 4 , PMEM_FILE_CREATE , 0666 , &mapped_len, &is_pmem);
        if (addr == NULL) {
            return -1;
        }
        unlink( path ) ;
        pmem_unmap(addr, mapped_len);
        return is_pmem;
    }
    void *addr = pmem_map_file(path, 0 , 0 , 0 , &mapped_len, &is_pmem);    
    if( addr == NULL) {
        return -1 ;
    }
    pmem_unmap(addr, mapped_len);
    return is_pmem;
} 

void pmem_custom_drain(){
    if( use_dsa ) batch.wait() ;
    else pmem_drain() ;
}

// 初始化持久内存文件
PMFileHandler* pm_file_open(const char *path ) {
    PMFileHandler *handler = (PMFileHandler*) malloc(sizeof(PMFileHandler)) ;
    handler->file_path = path ; 
    handler->extend_size = 128 * MB ; 
    struct stat st ;
    if( stat( path , &st ) == -1 ) { 
        printf( "file %s not exist\n" , path ) ; fflush( stdout ) ;
        handler->used_size = 0 ;
        handler->file_size = handler->extend_size ;
    } else { 
        handler->used_size = st.st_size ;
        handler->file_size = st.st_size ;
        printf( "file %s exist, size = %lu\n" , path , handler->used_size ) ; fflush( stdout ) ;
    }

    // 映射文件到内存
    size_t mapped_len;
    handler->pmem_addr = (char*)pmem_map_file(path, handler->file_size , 
                                PMEM_FILE_CREATE, 0666, &mapped_len , &handler->is_pmem);
    if (handler->pmem_addr == NULL) {
        printf( "pmem file init %s failed\n" , path ) ; fflush( stdout ) ;
        free( handler ) ;
        return NULL ;
    }
    handler->file_size = mapped_len ;
    printf( "open %s, mapped_len = %lu, file_size = %lu\n" , path , mapped_len , handler->file_size ) ; fflush( stdout ) ;
    return handler; 
}

// 扩展文件大小
int pm_file_extend(PMFileHandler *handler , size_t extend_size = 0 ) { 
    size_t new_size = handler->file_size + handler->extend_size ; 
    if( extend_size ) new_size = handler->file_size + extend_size ; 
    pmem_custom_drain() ;
    pmem_unmap(handler->pmem_addr, handler->file_size);

    // 重新映射
    size_t mapped_len;
    handler->pmem_addr = (char*)pmem_map_file(handler->file_path, new_size, 
                                PMEM_FILE_CREATE, 0666, &mapped_len, &handler->is_pmem);
    if (handler->pmem_addr == NULL){
        printf( "pmem_file %s extend failed\n" , handler->file_path ) ; fflush( stdout ) ;
        return 0;
    }
    for( size_t i = handler->file_size ; i < new_size ; i += 4096 ) {
        handler->pmem_addr[i] = 0 ;
    }
    handler->file_size = mapped_len ;
    printf( "mapped_len = %lu, new_size = %lu\n" , mapped_len , new_size ) ; fflush( stdout ) ;
    return 1;
}

// 追加数据到文件
size_t pm_file_append(PMFileHandler *handler, const void *data, size_t len) { 
    if (handler->used_size + len > handler->file_size) {
        size_t delta = ( len + handler->used_size + handler->extend_size - 1 ) / handler->extend_size * handler->extend_size - handler->file_size ;
        if ( pm_file_extend( handler, delta ) == 0) {
            printf( "extend failed\n" ) ; fflush( stdout ) ;
            return 0; // 扩展失败
        }
    } 
    if( use_dsa ){
        batch.submit_memcpy( handler->pmem_addr + handler->used_size , data , len ) ;
    } else {
        memcpy( handler->pmem_addr + handler->used_size , data , len ) ;
        pmem_flush(handler->pmem_addr + handler->used_size , len ) ;
    } 
    handler->used_size += len;
    if( handler->used_size > handler->file_size ) {
        printf( "len = %lu, used_size = %lu, file_size = %lu\n" , len , handler->used_size - len , handler->file_size ) ; fflush( stdout ) ;
    }
    return len;
}

// 关闭文件
void pm_file_close(PMFileHandler *handler) {
    if (handler) { 
        pmem_unmap(handler->pmem_addr, handler->file_size);
        int fd = open( handler->file_path, O_RDWR ) ; 
        if( fd == -1 ) {
            printf( "shrink %s failed\n" , handler->file_path ) ; fflush( stdout ) ;
            return ;
        }
        if( ftruncate( fd , handler->used_size ) != 0 ){
            printf( "shrink %s failed\n" , handler->file_path ) ; fflush( stdout ) ;
        }
        close( fd ) ; 
        free(handler);
    }
}

int single_len_max = 16384 , single_len_min = 512 ;
int main(){
    srand( 0 ) ;
    const char *src_path = "/mnt/pmemdir/dump.rdb" ;
    const char *path = "/mnt/pmemdir/dump.cp" ;
    // const char *path = "dump.cp" ;
    unlink( path ) ;
    // const char *path = "/mnt/pmemdir/testfile" ;
    printf( "%s %s" , path , check_if_path_is_pmem(path) ? "is pmem\n" : "not pmem\n" ) ; fflush( stdout ) ;

    uint64_t startns = timeStamp_hires() ;
    PMFileHandler *src = pm_file_open(src_path) ;
    if( check_if_path_is_pmem( path ) ) {
        PMFileHandler *handler = pm_file_open(path) ;
        if( handler == NULL ){
            printf( "open failed\n" ) ; fflush( stdout ) ;
            return 0 ;
        }
        volatile char x ;
        for( size_t i = 0 ; i < src->used_size ; i += 4096 ){
            x = *(src->pmem_addr + i) ;
        }  
        size_t src_size = src->used_size ;
        while( src_size ){
            size_t now_len = rand() % ( single_len_max - single_len_min + 1 ) + single_len_min ;
            if( now_len > src_size ) now_len = src_size ;
            if( pm_file_append( handler , src->pmem_addr + src->used_size - src_size , now_len ) == 0 ){
                printf( "write failed\n" ) ; fflush( stdout ) ; 
                return 0 ;
            }
            src_size -= now_len ;
        } 
        printf( "do_by_cpu_cnt = %d\n" , batch.db_task.do_by_cpu_cnt ) ; fflush( stdout ) ;
        printf( "batch_fail_cnt = %d\n" , batch.db_task.batch_fail_cnt ) ; fflush( stdout ) ;
        printf( "file_size = %lu, used_size = %lu\n" , handler->file_size , handler->used_size ) ; fflush( stdout ) ; 
        pmem_custom_drain() ;
        pm_file_close(handler) ;
    } else { 
        int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if( fd < 0 ) {
            printf( "open failed\n" ) ; fflush( stdout ) ;
            return 0 ;
        }
        size_t src_size = src->used_size ;
        while( src_size ){
            size_t now_len = rand() % ( single_len_max - single_len_min + 1 ) + single_len_min ;
            if( now_len > src_size ) now_len = src_size ;
            if( write( fd , src->pmem_addr + src->used_size - src_size , now_len ) < 0 ){
                printf( "write failed\n" ) ; fflush( stdout ) ;
                close( fd ) ;
                return 0 ;
            }
            src_size -= now_len ;
        } 
        close( fd ) ;
        printf( "file_size = %lu, used_size = %lu\n" , src->file_size , src->used_size ) ; fflush( stdout ) ; 
    }
    pm_file_close(src) ;
    uint64_t endns = timeStamp_hires() ;
    printf( "pmem copy time = %.3fs\n" , 1.0 * (endns - startns) / 1e9 ) ; fflush( stdout ) ;
    return 0 ;
}

// g++ pmem_test.cpp -o pmem_test -lpmem 