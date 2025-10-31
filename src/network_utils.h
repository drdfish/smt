#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>  // Windows �׽��ֱ��
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

// �����ƽ̨���׽��ֹرպ���
#ifdef _WIN32
#define close_socket closesocket
#define my_socket SOCKET
#else
#define close_socket close
#define my_socket int
#endif

// ���崫�������ݺͿ�������Ľṹ��
typedef struct {
    int type;           // ���ͣ�1 - �¶ȣ�2 - ʪ�ȣ�3 - ���յ�
    float value;        // ����ֵ
    char timestamp[20]; // ʱ���
} sensor_data_t;

typedef struct {
    int cmd;           // ��������
    char param[32];    // ����
} control_cmd_t;

void get_current_time(char* buffer);
// ��ƽ̨��˯�ߺ�������λΪ��
void platform_sleep(int seconds);

// ͨ�õ� IP ��ַת������
int my_inet_pton(int af, const char* src, void* dst);

// �׽��ֲ�����������
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
