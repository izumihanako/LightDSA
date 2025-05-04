
set(CMAKE_CXX_STANDARD 17)

# compile DSA_speed.cpp
add_executable( DSA_speed DSA_speed.cpp )
target_compile_features( DSA_speed PRIVATE cxx_std_17)   
target_compile_options( DSA_speed PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( DSA_speed dsa_memcpy )