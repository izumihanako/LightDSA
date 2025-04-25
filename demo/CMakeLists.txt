
# compile speedtest_memmove
add_executable( speedtest_memmove speedtest_memmove.cpp )
target_compile_options( speedtest_memmove PRIVATE -Wall -Wextra -mcmodel=large -O2 ) 
target_link_libraries( speedtest_memmove dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

# compile speedtest_memfill
add_executable( speedtest_memfill speedtest_memmove.cpp )
target_compile_options( speedtest_memfill PRIVATE -Wall -Wextra -mcmodel=large -O2 ) 
target_link_libraries( speedtest_memfill dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

# compile speedtest_compval
add_executable( speedtest_compval speedtest_compval.cpp )
target_compile_options( speedtest_compval PRIVATE -Wall -Wextra -mcmodel=large -O2 ) 
target_link_libraries( speedtest_compval dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

# compile coro_demo 
add_executable( coro_demo coro_demo.cpp )
target_compile_options( coro_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( coro_demo dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

# compile vector_demo
add_executable( vector_demo vector_demo.cpp )
target_compile_options( vector_demo PRIVATE -Wall -Wextra -O2 -Wno-unused-result )
target_link_libraries( vector_demo dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

# compile mergesort_demo
add_executable( mergesort_demo mergesort_demo.cpp )
target_compile_options( mergesort_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( mergesort_demo dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} ) 

# compile pmem_demo
add_executable( pmem_demo pmem_demo.cpp )
target_compile_options( pmem_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( pmem_demo dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} ${PMEM_LIBRARY} )