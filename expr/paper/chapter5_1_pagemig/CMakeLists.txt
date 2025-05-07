# compile page_mig_dsa.cpp
add_executable( page_mig_dsa page_mig_dsa.cpp )
target_compile_options( page_mig_dsa PRIVATE -Wall -Wextra -O2 -Wno-unused-result )
target_link_libraries( page_mig_dsa dsa_memcpy ${NUMA_LIBRARY} ) 