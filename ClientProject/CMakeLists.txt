# Client 目录 CMakeLists
set(CLIENT_SOURCES
    Client.c
)

add_executable(client ${CLIENT_SOURCES})

target_link_libraries(client PRIVATE network_utils)

# Windows 平台需要链接 ws2_32
if (WIN32)
    target_link_libraries(client PRIVATE ws2_32)
endif()

