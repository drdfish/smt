#include "network_utils.h"


void platform_sleep(int seconds) {
#ifdef _WIN32
    Sleep(seconds * 1000);  // Windows Sleep���Ժ���Ϊ��λ
#else
    sleep(seconds);  // Linux ��ʹ�� sleep
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

my_socket create_tcp_socket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
    }
    return sock;
}

my_socket create_udp_socket() {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
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

// Windows�µ�recv����
int recv_data(my_socket sockfd, void* buf, size_t len, int flags) {
    return recv(sockfd, ( char*)buf, (int)len, flags);
}

// Windows�µ�send����
int send_data(my_socket sockfd, const void* buf, size_t len, int flags) {
    return send(sockfd, (const char*)buf, (int)len, flags);
}

int send_data_with_addr(my_socket sockfd, const void* buf, size_t len, int flags, struct sockaddr_in* addr, socklen_t addr_len) {
    return sendto(sockfd, (const char*)buf, (int)len, flags, (struct sockaddr*)addr, addr_len);
}
#else 

// Linuxƽ̨�ĺ���ʵ��
int network_init() {
    return 1;
}

void network_cleanup() {
}

my_socket create_tcp_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
    }
    return sock;
}

my_socket create_udp_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
    }
    return sock;
}

int bind_socket(my_socket sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

int listen_socket(my_socket sockfd, int backlog) {
    return listen(sockfd, backlog);
}

int accept_connection(my_socket sockfd, struct sockaddr* addr, socklen_t * addrlen) {
    return accept(sockfd, addr, addrlen);
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