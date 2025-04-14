
set(CMAKE_CXX_STANDARD 17)

# compile align_for_speed
add_executable( align_for_speed align_for_speed.cpp )
target_compile_features( align_for_speed PRIVATE cxx_std_17)
target_compile_options( align_for_speed PRIVATE -Wall -Wextra -O2  -mcmodel=large )
target_link_libraries( align_for_speed dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )