#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>  // Windows 套接字编程
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// 定义跨平台的套接字关闭函数
#ifdef _WIN32
#define close_socket closesocket
#define my_socket SOCKET
#else
#define close_socket close
#define my_socket int
#endif

// 定义传感器数据和控制命令的结构体
typedef struct {
    int type;           // 类型：1 - 温度，2 - 湿度，3 - 光照等
    float value;        // 数据值
    char timestamp[20]; // 时间戳
} sensor_data_t;

typedef struct {
    int cmd;           // 命令类型 0 - 断开，1 - 数据请求，2 - 控制命令，3 - 登录
    char param[32];    // 参数
} control_cmd_t;

typedef struct {
    char device_id[5];               // 唯一标识符

    int type;                  // 数据类型标识：1 - sensor_data, 2 - control_cmd
    union {
        sensor_data_t sensor_data;   // 用于存储传感器数据
        control_cmd_t control_cmd;   // 用于存储控制命令
    } data;

} device_data_t;

typedef enum {
    MSG_TYPE_UNKNOWN,
    MSG_TYPE_SENSOR_DATA_RESPONSE, // 对获取传感器数据请求的响应
    MSG_TYPE_COMMAND_RESULT,       // 对执行远程命令的响应
    MSG_TYPE_DEVICE_LIST_RESPONSE, // 对列出设备请求的响应
    MSG_TYPE_CONFIG_RESPONSE,      // 对配置设备请求的响应
    MSG_TYPE_ALARM                 // 服务器主动发送的告警
} server_message_type_t;

// 服务器告警结构体
typedef struct {
    char device_id[32];
    char alarm_info[256];
    char timestamp[20];
} alarm_data_t;

// 通用服务器消息结构体
typedef struct {
    server_message_type_t type;
    union {
        sensor_data_t sensor_data;
        char command_result[BUFFER_SIZE];
        char device_list[BUFFER_SIZE];
        char config_response[128];
        alarm_data_t alarm_data;
    } data;
} server_message_t;

void init_console();

void get_current_time(char* buffer);
// 跨平台的睡眠函数，单位为秒
void platform_sleep(int seconds);

// 套接字操作函数声明
int network_init();
void network_cleanup();
my_socket create_tcp_socket(int *success);
my_socket create_udp_socket(int *success);
int bind_socket(my_socket sockfd, const struct sockaddr* addr, int addrlen);
int listen_socket(my_socket sockfd, int backlog);
int accept_connection(my_socket sockfd, struct sockaddr* addr, int* addrlen);
int recv_data(my_socket sockfd, void* buf, size_t len, int flags);
int send_data(my_socket sockfd, const void* buf, size_t len, int flags);
int send_data_with_addr(my_socket sockfd, const void* buf, size_t len, int flags, struct sockaddr_in* addr, socklen_t addr_len);
#endif // NETWORK_UTILS_H
