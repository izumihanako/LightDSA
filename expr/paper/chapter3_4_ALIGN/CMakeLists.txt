
set(CMAKE_CXX_STANDARD 17)

# compile alignment_speed
add_executable( alignment_speed alignment_speed.cpp )
target_compile_features( alignment_speed PRIVATE cxx_std_17)   
target_compile_options( alignment_speed PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( alignment_speed lightDSA )