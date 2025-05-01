
set(CMAKE_CXX_STANDARD 17)

# compile page_tapping
add_executable( page_tapping page_tapping.cpp )
target_compile_features( page_tapping PRIVATE cxx_std_17)
target_compile_options( page_tapping PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( page_tapping dsa_memcpy )

# compile page_fault_memmove_speed
add_executable( page_fault_memmove_speed page_fault_memmove_speed.cpp )
target_compile_features( page_fault_memmove_speed PRIVATE cxx_std_17)
target_compile_options( page_fault_memmove_speed PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( page_fault_memmove_speed dsa_memcpy )