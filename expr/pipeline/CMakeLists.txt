
set(CMAKE_CXX_STANDARD 17)

# compile trans_pipe 
add_executable( trans_pipe trans_pipe.cpp )
target_compile_features( trans_pipe PRIVATE cxx_std_17)   
target_compile_options( trans_pipe PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( trans_pipe dsa_memcpy )

# compile in_batch_pipe 
add_executable( in_batch_pipe in_batch_pipe.cpp )
target_compile_features( in_batch_pipe PRIVATE cxx_std_17)   
target_compile_options( in_batch_pipe PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( in_batch_pipe dsa_memcpy )

# compile engine_ooo 
add_executable( engine_ooo engine_ooo.cpp )
target_compile_features( engine_ooo PRIVATE cxx_std_17)   
target_compile_options( engine_ooo PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( engine_ooo dsa_memcpy )