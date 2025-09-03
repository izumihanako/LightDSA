# compile coro_demo, this need Boost library
# --------------------------------------------------------------
# # find Boost lib
# find_package( Boost 1.74.0 REQUIRED context coroutine )
# include_directories( ${Boost_INCLUDE_DIRS} )
# add_executable( coro_demo coro_demo.cpp )
# target_compile_options( coro_demo PRIVATE -Wall -Wextra -O2 )
# target_link_libraries( coro_demo lightDSA ${Boost_LIBRARIES} )
# --------------------------------------------------------------

# compile mergesort_demo
add_executable( mergesort_demo mergesort_demo.cpp )
target_compile_options( mergesort_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( mergesort_demo lightDSA ) 

# compile pmem_demo
add_executable( pmem_demo pmem_demo.cpp )
target_compile_options( pmem_demo PRIVATE -Wall -Wextra -O2 )
target_link_libraries( pmem_demo lightDSA ${PMEM_LIBRARY} )
# target_link_options( pmem_demo PRIVATE -lm )

# compile example_c
add_executable( example_c example_c.c )
set_target_properties( example_c PROPERTIES LINKER_LANGUAGE C ) 
target_compile_options( example_c PRIVATE -Wall -Wextra -O2 )
target_link_libraries( example_c PRIVATE lightDSA )

# compile example_cpp
add_executable( example_cpp example_cpp.cpp )
target_compile_options( example_cpp PRIVATE -Wall -Wextra -O2 )
target_link_libraries( example_cpp PRIVATE lightDSA )