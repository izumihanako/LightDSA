# compile codes for
#   Motivation 
add_subdirectory( chapter1_Motivation/ )

# compile codes for
#   Intel Data Streaming Accelerator
#     -> Operations and Performance
add_subdirectory( chapter2_3_DSAperf/ )

# compile codes for
#   In-Depth Exploration of DSA features
#     -> Address Translation Mechanisms
add_subdirectory( chapter3_1_ATC/ )

# compile codes for
#   In-Depth Exploration of DSA features
#     -> Page Fault Handling
add_subdirectory( chapter3_2_PF/ )

# compile codes for
#   In-Depth Exploration of DSA features
#     -> Pipeline Mechanisms
add_subdirectory( chapter3_3_PIPE/ )

# compile codes for
#   In-Depth Exploration of DSA features
#     -> Write Address Alignment
add_subdirectory( chapter3_4_ALIGN/ )

# compile codes for
#   In-Depth Exploration of DSA features
#     -> Out-of-Order Execution
add_subdirectory( chapter3_5_OOO/ )

# compile codes for
#   Evaluation 
#     -> microbench -> vector
add_subdirectory( chapter5_1_vector/ )

# compile codes for
#   Evaluation 
#     -> microbench -> page migration
add_subdirectory( chapter5_1_pagemig/ )

# compile codes for
#   Evaluation 
#     -> microbench -> shuffle
add_subdirectory( chapter5_1_shuffle/ )