#include "network_utils.h"



void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void platform_sleep(int seconds) {
#ifdef _WIN32
    Sleep(seconds * 1000);  // Windows Sleep是以毫秒为单位
#else
    sleep(seconds);  // Linux 下使用 sleep
#endif
}

void get_current_time(char* buffer) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", t);
}

#ifdef _WIN32
// Windows
int network_init() {
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        printf("WSAStartup failed: %d\n", res);
        return -1;
    }
    return 1;
}

void network_cleanup() {
    WSACleanup();
}

my_socket create_tcp_socket(int* success) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    *success = 1;
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        *success = -1;
    }

    return sock;
}

my_socket create_udp_socket(int* success) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    *success = 1;
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        *success = -1;
    }

    return sock;
}

int bind_socket(my_socket sockfd, const struct sockaddr* addr, int addrlen) {
    return bind(sockfd, addr, addrlen);
}

int listen_socket(my_socket sockfd, int backlog) {
    return listen(sockfd, backlog);
}

int accept_connection(my_socket sockfd, struct sockaddr* addr, int* addrlen) {
    return accept(sockfd, addr, addrlen);
}

// Windows下的recv函数
int recv_data(my_socket sockfd, void* buf, size_t len, int flags) {
    return recv(sockfd, ( char*)buf, (int)len, flags);
}

// Windows下的send函数
int send_data(my_socket sockfd, const void* buf, size_t len, int flags) {
    return send(sockfd, (const char*)buf, (int)len, flags);
}

int send_data_with_addr(my_socket sockfd, const void* buf, size_t len, int flags, struct sockaddr_in* addr, socklen_t addr_len) {
    return sendto(sockfd, (const char*)buf, (int)len, flags, (struct sockaddr*)addr, addr_len);
}
#else

// Linux平台的函数实现
int network_init() {
    return 1;
}

void network_cleanup() {
}

my_socket create_tcp_socket(int* success) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    *success = 1;
    if (sock < 0) {
        perror("socket failed");
        *success = -1;
    }

    return sock;
}

my_socket create_udp_socket(int* success) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    *success = 1;
    if (sock < 0) {
        perror("socket failed");
        *success = -1;
    }
    return sock;
}

// <--- 修改：将addrlen的类型从 socklen_t 改为 int，以匹配头文件声明
int bind_socket(my_socket sockfd, const struct sockaddr* addr, int addrlen) {
    // 注意：这里将int隐式转换为socklen_t，在大多数平台上是安全的
    return bind(sockfd, addr, addrlen);
}

int listen_socket(my_socket sockfd, int backlog) {
    return listen(sockfd, backlog);
}

// <--- 修改：将addrlen的类型从 socklen_t* 改为 int*，以匹配头文件声明
int accept_connection(my_socket sockfd, struct sockaddr* addr, int* addrlen) {
    // 注意：调用真实的accept函数时，需要将 int* 强制转换为 socklen_t*
    return accept(sockfd, addr, (socklen_t*)addrlen);
}

int recv_data(my_socket sockfd, void* buf, size_t len, int flags) {
    return recv(sockfd, buf, len, flags);
}

int send_data(my_socket sockfd, const void* buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
}

int send_data_with_addr(my_socket sockfd, const void* buf, size_t len, int flags, struct sockaddr_in* addr, socklen_t addr_len) {
    return sendto(sockfd, buf, len, flags, (struct sockaddr*)addr, addr_len);
}
#endif