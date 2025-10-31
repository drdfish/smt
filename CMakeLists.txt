cmake_minimum_required(VERSION 3.15)
# 项目名称
project(tcp_udp C)

# --- 指定所有可执行文件的输出目录 ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


# 添加子工程
add_subdirectory(src)
add_subdirectory(ClientProject)
add_subdirectory(ServerProject)