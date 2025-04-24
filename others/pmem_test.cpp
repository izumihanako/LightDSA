#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/details/dsa_constant.hpp"

typedef struct {
    size_t file_size;     // 当前文件总大小
    size_t used_size;     // 已使用的空间大小
    size_t extend_size;   // 每次扩展的大小 (例如 4MB)
    const char* file_path;     // 文件路径
    char *pmem_addr;      // 映射的持久内存地址
    int is_pmem;          // 是否为真正的持久内存
} PMFileHandler;

size_t getFileSize(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        return 0; // 文件不存在
    }
    return st.st_size ;
}

// 初始化持久内存文件
PMFileHandler* pm_file_open(const char *path ) {
    PMFileHandler *handler = (PMFileHandler*) malloc(sizeof(PMFileHandler)) ;
    handler->file_path = path ;
    if (!handler) { printf( "mmap for PMFileHandler failed\n" ) ; fflush( stdout ) ; return NULL; } 
    handler->used_size = getFileSize(path) ;
    handler->extend_size = 4 * MB ;
    size_t mmap_size = handler->used_size ? handler->used_size : handler->extend_size ;
 
    // 映射文件到内存
    size_t mapped_len;
    handler->pmem_addr = (char*)pmem_map_file(path, mmap_size, 
                                PMEM_FILE_CREATE, 0666, &mapped_len , &handler->is_pmem);
    if (handler->pmem_addr == NULL) {
        printf( "pmem_map_file failed\n" ) ; fflush( stdout ) ;
        goto fail;
    }
    handler->file_size = getFileSize(path) ;
    printf( "mapped_len = %lu, file_size = %lu\n" , mapped_len , handler->file_size ) ; fflush( stdout ) ;
    return handler;
fail: 
    free(handler);
    return NULL;
}

void pm_file_persist_offset_len( PMFileHandler *handler , size_t offset , size_t len ) {
    if (handler->is_pmem) {
        pmem_persist(handler->pmem_addr + offset, len);
    } else {
        pmem_msync(handler->pmem_addr + offset, len);
    }
}

// 扩展文件大小
int pm_file_extend(PMFileHandler *handler , size_t extend_size = 0 ) { 
    size_t new_size = handler->file_size + handler->extend_size ; 
    if( extend_size ) new_size = handler->file_size + extend_size ; 
    pmem_unmap(handler->pmem_addr, handler->file_size);

    // 重新映射
    size_t mapped_len;
    handler->pmem_addr = (char*)pmem_map_file(handler->file_path, new_size, 
                                PMEM_FILE_CREATE, 0666, &mapped_len, &handler->is_pmem);
    printf( "mapped_len = %lu, new_size = %lu\n" , mapped_len , new_size ) ; fflush( stdout ) ;
    if (handler->pmem_addr == NULL) return -1;
    handler->file_size = new_size;
    return 0;
}

int pm_file_shrink_to_used( PMFileHandler *handler ){
    if( handler->used_size == handler->file_size ) return 0 ; 
    pmem_unmap(handler->pmem_addr, handler->file_size) ;
    
    // 重新映射
    size_t mapped_len;
    handler->pmem_addr = (char*)pmem_map_file(handler->file_path, handler->used_size, 
                                PMEM_FILE_CREATE, 0666, &mapped_len, &handler->is_pmem);
    printf( "mapped_len = %lu, used_size = %lu\n" , mapped_len , handler->used_size ) ; fflush( stdout ) ;
    if (handler->pmem_addr == NULL) return -1;
    handler->file_size = handler->used_size;
    return 0;
}

// 追加数据到文件
size_t pm_file_append_persist(PMFileHandler *handler, const void *data, size_t len) { 
    if (handler->used_size + len > handler->file_size) {
        size_t delta = ( len + handler->file_size - handler->used_size + handler->extend_size - 1 ) / handler->extend_size * handler->extend_size - handler->file_size ;
        if (pm_file_extend( handler, delta ) != 0) {
            printf( "extend failed\n" ) ; fflush( stdout ) ;
            return 0; // 扩展失败
        }
    }
    memcpy(handler->pmem_addr + handler->used_size, data, len);
    pm_file_persist_offset_len(handler, handler->used_size, len ); // 确保数据持久化
    handler->used_size += len;
    return len;
}

// 关闭文件
void pm_file_close(PMFileHandler *handler) {
    if (handler) { 
        pm_file_shrink_to_used(handler) ;
        pmem_unmap(handler->pmem_addr, handler->file_size);
        free(handler);
    }
}

int main(){
    const char *path = "/mnt/pmemdir/testfile" ;
    PMFileHandler *handler = pm_file_open(path) ;
    if( handler == NULL ){
        printf( "open failed\n" ) ; fflush( stdout ) ;
        return 0 ;
    }
    // read file 
    
    for( int i = 0 ; i < handler->used_size ; i ++ ){
        printf( "%c" , handler->pmem_addr[i] ) ; fflush( stdout ) ;
    }

    // write file
    pm_file_append_persist( handler , "hello world\n" , 13 ) ;

    printf( "file_size = %lu, used_size = %lu\n" , handler->file_size , handler->used_size ) ; fflush( stdout ) ;
    pm_file_close(handler) ;
    return 0 ;
}