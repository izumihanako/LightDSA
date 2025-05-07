# compile speedtest_memmove
add_executable( speedtest_memmove speedtest_memmove.cpp )
target_compile_options( speedtest_memmove PRIVATE -Wall -Wextra -mcmodel=large -O2 ) 
target_link_libraries( speedtest_memmove dsa_memcpy ${Boost_LIBRARIES} ${NUMA_LIBRARY} ) 
# compile speedtest_memfill
add_executable( speedtest_memfill speedtest_memfill.cpp )
target_compile_options( speedtest_memfill PRIVATE -Wall -Wextra -mcmodel=large -O2 ) 
target_link_libraries( speedtest_memfill dsa_memcpy ${Boost_LIBRARIES} )

# compile speedtest_compval
add_executable( speedtest_compval speedtest_compval.cpp )
target_compile_options( speedtest_compval PRIVATE -Wall -Wextra -mcmodel=large -O2 ) 
target_link_libraries( speedtest_compval dsa_memcpy ${Boost_LIBRARIES} )

# compile coro_demo 
add_executable( coro_demo coro_demo.cpp )
target_compile_options( coro_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( coro_demo dsa_memcpy ${Boost_LIBRARIES} )

# compile mergesort_demo
add_executable( mergesort_demo mergesort_demo.cpp )
target_compile_options( mergesort_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( mergesort_demo dsa_memcpy ) 

# compile pmem_demo
add_executable( pmem_demo pmem_demo.cpp )
target_compile_options( pmem_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( pmem_demo dsa_memcpy ${PMEM_LIBRARY} )
# target_link_options( pmem_demo PRIVATE -lm )

# compile c_demo
add_executable( c_demo c_demo.c )
set_target_properties( c_demo PROPERTIES LINKER_LANGUAGE C ) 
target_compile_options( c_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( c_demo PRIVATE dsa_memcpy )