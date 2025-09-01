# compile coro_demo, this need Boost library
# --------------------------------------------------------------
# # find Boost lib
# find_package( Boost 1.74.0 REQUIRED context coroutine )
# include_directories( ${Boost_INCLUDE_DIRS} )
# add_executable( coro_demo coro_demo.cpp )
# target_compile_options( coro_demo PRIVATE -Wall -Wextra -O2 )
# target_link_libraries( coro_demo dsa_memcpy ${Boost_LIBRARIES} )
# --------------------------------------------------------------

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