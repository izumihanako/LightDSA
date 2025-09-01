# compile vector_dsa
add_executable( vector_dsa vector_dsa.cpp )
target_compile_options( vector_dsa PRIVATE -Wall -Wextra -O2 -Wno-unused-result )
target_link_libraries( vector_dsa lightDSA )