# compile page_fault_speed
add_executable( page_fault_speed page_fault_speed.cpp )
target_compile_options( page_fault_speed PRIVATE -Wall -Wextra -O0 -mcmodel=large -fno-omit-frame-pointer )
target_link_libraries( page_fault_speed dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

add_executable( dedicated_remap dedicated_remap.cpp )
target_compile_options( dedicated_remap PRIVATE -Wall -Wextra -O0 -mcmodel=large -fno-omit-frame-pointer )
target_link_libraries( dedicated_remap dsa_memcpy ${Boost_LIBRARIES} ${accel_config_LIBRARY} )

# compile align
add_subdirectory( align/ )

# compile cache
add_subdirectory( cache/ )

# compile pipeline
add_subdirectory( pipeline/ )