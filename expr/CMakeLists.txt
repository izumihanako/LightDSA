# compile page_fault_speed
add_executable( page_fault_speed page_fault_speed.cpp )
target_compile_options( page_fault_speed PRIVATE -Wall -Wextra -O0 -mcmodel=large -fno-omit-frame-pointer )
target_link_libraries( page_fault_speed lightDSA )

add_executable( dedicated_remap dedicated_remap.cpp )
target_compile_options( dedicated_remap PRIVATE -Wall -Wextra -O0 -mcmodel=large -fno-omit-frame-pointer )
target_link_libraries( dedicated_remap lightDSA ${accel_config_LIBRARY})

# compile align
add_subdirectory( align/ )

# compile pipeline
add_subdirectory( pipeline/ )

# compile paper
add_subdirectory( paper/ )