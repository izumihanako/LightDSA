
set(CMAKE_CXX_STANDARD 17)

# compile pipeline_mc
add_executable( pipeline_mc pipeline_mc.cpp )
target_compile_features( pipeline_mc PRIVATE cxx_std_17)   
target_compile_options( pipeline_mc PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( pipeline_mc lightDSA )

# compile pipeline_mc_plus
add_executable( pipeline_mc_plus pipeline_mc_plus.cpp )
target_compile_features( pipeline_mc_plus PRIVATE cxx_std_17)   
target_compile_options( pipeline_mc_plus PRIVATE -Wall -Wextra -O0 -mcmodel=large )
target_link_libraries( pipeline_mc_plus lightDSA )