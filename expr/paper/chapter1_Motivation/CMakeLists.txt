
set(CMAKE_CXX_STANDARD 17)

# compile motivation.cpp
add_executable( motivation motivation.cpp )
target_compile_features( motivation PRIVATE cxx_std_17)   
target_compile_options( motivation PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( motivation lightDSA )

# compile motivation_speed_test.cpp
add_executable( motivation_speed_test motivation_speed_test.cpp )
target_compile_features( motivation_speed_test PRIVATE cxx_std_17)   
target_compile_options( motivation_speed_test PRIVATE -Wall -Wextra -O2 -mcmodel=large )
target_link_libraries( motivation_speed_test lightDSA )