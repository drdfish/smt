add_library(network_utils
    network_utils.h
    network_utils.c
)

target_include_directories(network_utils
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)