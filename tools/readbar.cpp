#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>

void printbits( void* addr , int bitcnt ){  
    int finbit = 0 ;
    while( bitcnt ){
        for( int i = 0 ; i <= 64 && i <= bitcnt ; i ++ ){
            if( i % 8 == 1 ){
                int tmp = finbit + i - 1 , wid = 0 ;
                while( tmp ) tmp /= 10 , wid ++ ;
                for( int widtmp = wid ; widtmp > 1 ; widtmp -= 2 ) printf( "\b" ) ;
                printf( "%d" , finbit + i - 1 ) ; 
                for( int widtmp = wid - 1 ; widtmp > 1 ; widtmp -= 2 ) i ++ ;
            }
            printf( " " ) ;
        } puts( "" ) ;

        for( int i = 1 ; i < 64 && i < bitcnt ; i ++ ) printf( ( i % 8 == 1 ) ? "|v" : "-" ) ; 
        puts( "-" ) ;
        
        for( int i = 0 ; i < 64 && bitcnt ; i ++ , bitcnt -- ){ 
            int byte = ( finbit + i ) / 8 ; 
            if( i % 8 == 0 ) printf( "|" ) ;
            if( ( *((uint8_t*)addr+byte) ) & ( 1 << (i%8) ) ) printf( "1" ) ;
            else printf( "0" ) ;
        }
        puts( "|" ) ; 
        finbit += 64 ;
    } 
}

void writebit( void *addr , int bitpos , bool val ){
    int bytepos = bitpos / 8 ;
    uint8_t byteval = *((uint8_t*)addr + bytepos) ;
    bitpos %= 8 ;
    byteval = byteval - ( byteval & ( 1 << bitpos ) ) + ( val << bitpos ) ;
    *((uint8_t*)addr + bytepos) = byteval ;
}

int main(){
    //  The file handle can be found by typing "lspci -v "
    //  looking for your device.
    int fd = open("/sys/devices/pci0000:e7/0000:e7:01.0/resource0", O_RDWR | O_SYNC);
    //  mmap returns a userspace address 
    void* ptr = mmap(0, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    printf( "PCI BAR0 at addr:\n%p\n" , ptr );

    void *VERSION = (char*)ptr + 0x00 ;
    printf( "VERSION at addr %p:\n" , VERSION ) ;
    printbits( VERSION , 16 ) ;
    puts( "" ) ; 

    void *WQCAP = (char*)ptr + 0x20 ;
    printf( "WQCAP at addr %p:\n" , WQCAP ) ;
    printbits( WQCAP , 64 ) ;
    puts( "" ) ;

    void *GRPCAP = (char*)ptr + 0x30 ;
    printf( "GRPCAP at addr %p:\n" , GRPCAP ) ;
    printbits( GRPCAP , 64 ) ;
    puts( "" ) ;    
    
    void *ENGCAP = (char*)ptr + 0x38 ;
    printf( "ENGCAP at addr %p:\n" , ENGCAP ) ;
    printbits( ENGCAP , 64 ) ;
    puts( "" ) ;

    void* OPCAP = (char*)ptr + 0x40 ;
    printf( "OPCAP at addr %p:\n" , OPCAP ) ;
    printbits( OPCAP , 256 ) ;
    puts( "" ) ;
    
    void* Table_offsets = (char*)ptr + 0x60 ;
    printf( "Table_offsets at addr %p:\n" , Table_offsets ) ;
    printbits( Table_offsets , 96 ) ;
    puts( "" ) ;
    uint32_t Group_Configuration_Offset = (*(uint16_t*)((uintptr_t)Table_offsets))*0x100 ;
    printf( "Group Configuration Offset : %u\n" , Group_Configuration_Offset ) ; 
    uint32_t WQ_Configuration_Offset = (*(uint16_t*)((uintptr_t)Table_offsets+2))*0x100 ;
    printf( "WQ Configuration Offset : %u\n" , WQ_Configuration_Offset ) ; 
    uint32_t MSI_X_Offset = (*(uint16_t*)((uintptr_t)Table_offsets+4))*0x100 ;
    printf( "MSI_X_Offset : %u\n" , MSI_X_Offset ) ; 
    uint32_t IMS_Offset = (*(uint16_t*)((uintptr_t)Table_offsets+6))*0x100 ;
    printf( "IMS Offset : %u\n" , IMS_Offset) ; 
    uint32_t Perfmon_Offset = (*(uint16_t*)((uintptr_t)Table_offsets+8))*0x100 ;
    printf( "Perfmon Offset : %u\n" , Perfmon_Offset ) ;  
    uint32_t IDP_Offset = (*(uint16_t*)((uintptr_t)Table_offsets+10))*0x100 ;
    printf( "Inter-Domain Permissions Table Offset : %u\n" , IDP_Offset ) ; 

    void* PERFCAP = (char*)ptr + Perfmon_Offset ;
    printf( "PERFCAP at addr %p:\n" , PERFCAP ) ;
    printbits( PERFCAP , 64 ) ;
}

// g++ readbar.cpp -o readbar