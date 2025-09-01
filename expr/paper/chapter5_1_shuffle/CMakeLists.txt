# compile shuffle_dsa
add_executable( shuffle_dsa shuffle_dsa.cpp )
target_compile_options( shuffle_dsa PRIVATE -Wall -Wextra -O2 -Wno-unused-result )
target_link_libraries( shuffle_dsa lightDSA ${NUMA_LIBRARY} )