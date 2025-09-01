
set(CMAKE_CXX_STANDARD 17)

# compile diff_desc_noop
add_executable( diff_desc_noop diff_desc_noop.cpp )
target_compile_features( diff_desc_noop PRIVATE cxx_std_17)   
target_compile_options( diff_desc_noop PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( diff_desc_noop lightDSA )

# compile diff_desc_s_src_d_dst
add_executable( diff_desc_s_src_d_dst diff_desc_s_src_d_dst.cpp )
target_compile_features( diff_desc_s_src_d_dst PRIVATE cxx_std_17)   
target_compile_options( diff_desc_s_src_d_dst PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( diff_desc_s_src_d_dst lightDSA )

# compile diff_desc_same_mem
add_executable( diff_desc_same_mem diff_desc_same_mem.cpp )
target_compile_features( diff_desc_same_mem PRIVATE cxx_std_17)  
target_compile_options( diff_desc_same_mem PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( diff_desc_same_mem lightDSA )

# compile same_desc_diff_mem
add_executable( same_desc_diff_mem same_desc_diff_mem.cpp )
target_compile_features( same_desc_diff_mem PRIVATE cxx_std_17)   
target_compile_options( same_desc_diff_mem PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( same_desc_diff_mem lightDSA )

# compile diff_desc_diff_mem
add_executable( diff_desc_diff_mem diff_desc_diff_mem.cpp )
target_compile_features( diff_desc_diff_mem PRIVATE cxx_std_17)
target_compile_options( diff_desc_diff_mem PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( diff_desc_diff_mem lightDSA )