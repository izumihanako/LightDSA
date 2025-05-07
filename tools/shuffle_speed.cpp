#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <chrono>
using namespace std;

uint64_t timeStamp_hires(){ 
    //Cast the time point to ms, then get its duration, then get the duration's count.
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();  
}
constexpr int data_size = 256 ;
struct Data{
    char arr[data_size] ;
} ;
 
int myrandom (int i) { return std::rand()%i;}

int cnt = 1000000 ;
int main(){
    vector<Data> data ;
    for( int i = 0 ; i < cnt ; i ++ ){
        Data x ;
        for( int j = 0 ; j < data_size ; j ++ ){
            x.arr[j] = rand() % 256 ;
        }
        data.push_back( x ) ;
    } 
    uint64_t start = timeStamp_hires() , x = 0 ;
    for( int i = 0 ; i < cnt ; i ++ ){
        x ^= myrandom( i + 1 ) ;
    }
    uint64_t end = timeStamp_hires() ;
    double rand_time = ( end - start ) / 1000000000.0 ;
    printf( "rand time = %.3fGB/s\n" , ( cnt * sizeof(Data) ) / rand_time / 1000000000.0 ) ;

    start = timeStamp_hires() ;
    random_shuffle( data.begin() , data.end() , myrandom ) ;
    end = timeStamp_hires() ;
    double elapsed = ( end - start ) / 1000000000.0  ;
    printf( "speed = %.3fGB/s" , ( cnt * sizeof(Data) ) / elapsed / 1000000000.0 ) ;
}