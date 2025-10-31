# Server 目录 CMakeLists
set(SERVER_SOURCES
    Server.c
)

add_executable(server ${SERVER_SOURCES})

target_link_libraries(server PRIVATE network_utils)

# Windows 平台需要链接 ws2_32
if (WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif()
