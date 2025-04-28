#include <cstdio>
#include <cstring>

int main(){
    char *a = new char[12 * 1024 * 1024] ;
    char *b = new char[12 * 1024 * 1024] ;
    for( int i = 0 ; i < 12 * 1024 * 1024 ; i ++ ){
        a[i] = i ;
        b[i] = 0 ;
    }

    for( int i = 0 ; i < 12 * 1024 * 1024 ; i += 31 ){
        memcpy( b + i , a + i , 31 ) ;
    }
}