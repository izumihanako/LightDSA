
set(CMAKE_CXX_STANDARD 17)

# compile ooo_test 
add_executable( ooo_test ooo_test.cpp )
target_compile_features( ooo_test PRIVATE cxx_std_17)   
target_compile_options( ooo_test PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( ooo_test dsa_memcpy )