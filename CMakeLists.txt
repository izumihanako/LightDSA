cmake_minimum_required( VERSION 3.16 )
project( DSA_Memcpy ) 
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

### ------------------------ defines ------------------------ ###
# defs 
# option( FLAG_BOF    "option for desc flag BOF"  off )
# option( FLAG_CC     "option for desc flag CC "  off )
# option( FLAG_DRDBK  "option for desc flag RDBK, use it to ensure that data written has become persistent" off )
# option( opt_align64 "optimization 1: descs in batch src always align to 64B" off )

# add_definitions( -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} )
# message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE} )
# # apply defs 
# message(STATUS "FLAG_BOF    : " ${FLAG_BOF} ) 
# if( FLAG_BOF )
#     add_definitions( -DFLAG_BLOCK_ON_FAULT ) 
# endif()

# message(STATUS "FLAG_CC     : " ${FLAG_CC} )
# if( FLAG_CC )
#     add_definitions( -DFLAG_CACHE_CONTROL ) 
# endif()

# message(STATUS "FLAG_DRDBK  : " ${FLAG_DRDBK} )
# if( FLAG_DRDBK )
#     add_definitions( -DFLAG_DEST_READBACK ) 
# endif()

# message(STATUS "opt_align64 : " ${opt_align64} )
# if( opt_align64 )
#     add_definitions( -DDESCS_ADDRESS_ALIGNMENT ) 
# endif()

# if( BUILD_RELEASE )
#     add_definitions( -DBUILD_RELEASE )
# endif() 
### ------------------------ compile ------------------------ ###

# compile options
set(CMAKE_CXX_STANDARD 14) 
include_directories(${CMAKE_SOURCE_DIR})

# find Boost lib
find_package( Boost 1.74.0 REQUIRED context coroutine )
include_directories( ${Boost_INCLUDE_DIRS} )

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
target_compile_options( dsa_memcpy PRIVATE -fPIC -O2 -mclflushopt )
target_link_libraries( dsa_memcpy PRIVATE ${accel_config_LIBRARY} )

# compile demo
add_subdirectory( demo/ )

# compile experiment
add_subdirectory( expr/ )

# copy scripts
file( COPY scripts/ DESTINATION ${PROJECT_BINARY_DIR} )
