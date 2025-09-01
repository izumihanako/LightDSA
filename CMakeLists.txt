cmake_minimum_required( VERSION 3.16 )
project( DSA_Memcpy ) 
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
### ------------------------ compile ------------------------ ###

# compile options
set(CMAKE_CXX_STANDARD 14) 
include_directories(${CMAKE_SOURCE_DIR})

# find Boost lib
# find_package( Boost 1.74.0 REQUIRED context coroutine )
# include_directories( ${Boost_INCLUDE_DIRS} ) 
# ----------------NOTICE--------------------
# The boost is just used in demo/coro_demo, but not used in the LightDSA library.
# So we do not set Boost as a required package for the whole project.

# find NUMA lib
find_library(NUMA_LIBRARY numa REQUIRED)

# find libpmem
find_package(Pmem REQUIRED)

# find idxd-config lib 
SET( accel_config_libpath "/usr/lib64" )
find_library( accel_config_LIBRARY accel-config ${accel_config_libpath} )
message(STATUS "accel_config_LIBRARY: " ${accel_config_LIBRARY} )
if ( ${accel_config_LIBRARY} STREQUAL "accel_config_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR 
            "Required accel-config library but not found !\n"
            "Please confirm that the idxd-config library is installed correctly, and set the CMAKE variable accel_config_libpath to your installation path" )
endif()


# add src files
file( GLOB_RECURSE SRC_LIST    "${CMAKE_SOURCE_DIR}/src/*.cpp" )
file( GLOB_RECURSE HEADER_LIST "${CMAKE_SOURCE_DIR}/src/*.hpp" )

# list src files
# foreach(list ${SRC_LIST})
#     message( ${list} )
# endforeach(list)
# message( "" )
# foreach(list ${HEADER_LIST})
#     message( ${list} )  
# endforeach(list)

# generate .a
add_library( dsa_memcpy SHARED ${SRC_LIST} )
target_include_directories( dsa_memcpy PUBLIC ${CMAKE_SOURCE_DIR}/src )
target_compile_options( dsa_memcpy PRIVATE -fPIC -O2 -mclflushopt -mclwb )
target_link_libraries( dsa_memcpy PRIVATE ${accel_config_LIBRARY} )

# compile example
add_subdirectory( example/ )

# compile experiment
add_subdirectory( expr/ )

# copy scripts
file( COPY scripts/ DESTINATION ${PROJECT_BINARY_DIR} )
