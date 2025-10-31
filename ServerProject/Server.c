#include "network_utils.h" // 引入头文件

#define TCP_PORT 8080
#define UDP_PORT 8081
#define MAX_CLIENTS 10
#define MAX_SENSOR_DATA 10

my_socket client_sockets[MAX_CLIENTS];
sensor_data_t* client_sensor_data[MAX_CLIENTS];

void init_client_sockets() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1; // -1 表示该位置没有连接
    }
}

int find_empty_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            return i;
        }
    }
    return -1;  // 如果没有空位，返回 -1
}

void remove_client(int index) {
    if (index >= 0 && index < MAX_CLIENTS) {
        close_socket(client_sockets[index]);
        client_sockets[index] = -1;  // 清除套接字
        printf("客户端套接字 %d 连接已断开\n", index);
    }
    //释放内存
    if (client_sensor_data[index] != NULL) {
        free(client_sensor_data[index]);
        client_sensor_data[index] = NULL;
    }

}

// 处理 TCP 连接
int handle_tcp_connection(my_socket client_sock) {
    char buffer[BUFFER_SIZE];
    control_cmd_t cmd;

    int bytes_received = recv_data(client_sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) return -1;

    memcpy(&cmd, buffer, sizeof(cmd));

    switch (cmd.cmd)
    {
    case 1: // 获取传感器数据
    {
        printf("客户端请求传感器数据\n");

        // 模拟传感器数据
        sensor_data_t sensor_data = { 1, 25.5, "2025-10-24 10:00:00" };

        send_data(client_sock, &sensor_data, sizeof(sensor_data), 0);
        printf("传感器数据已发送\n");
        break;
    }

    case 2: // 控制设备
    {
        printf("执行控制命令: %s\n", cmd.param);

        char response[] = "控制命令执行成功";

        send_data(client_sock, response, strlen(response), 0);
        break;
    }
    case 0: //关闭连接
    {
        printf("执行关闭: %s\n", cmd.param);

        char response[] = "连接断开";

        send_data(client_sock, response, strlen(response), 0);
        return -1;
    }
    default:
    {
        printf("未知命令\n");
        break;
    }
    }

    return 0;
}

// 处理 UDP 数据
int handle_udp_data(my_socket udp_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
        (struct sockaddr*)&client_addr, &addr_len);

    if (bytes_received > 0) {
        sensor_data_t sensor_data;
        memcpy(&sensor_data, buffer, sizeof(sensor_data));

        printf("收到传感器数据 - 类型:%d 数值:%.2f 时间:%s\n",
            sensor_data.type, sensor_data.value, sensor_data.timestamp);

        // 数据存储逻辑
        FILE* log_file = fopen("sensor_log.txt", "a");
        if (log_file) {
            fprintf(log_file, "类型:%d, 数值:%.2f, 时间:%s\n",
                sensor_data.type, sensor_data.value, sensor_data.timestamp);
            fclose(log_file);
        }

        // 报警检查
        if (sensor_data.type == 1 && sensor_data.value > 30.0) {
            printf("警告: 温度过高! 当前温度: %.2f\n", sensor_data.value);
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    my_socket tcp_sock, udp_sock;
    struct sockaddr_in tcp_addr, udp_addr;
    fd_set read_fds;
    int max_fd;

    int tcp_port = TCP_PORT;
    int udp_port = UDP_PORT;

    // 设置 tcp udp 端口
    if (argc > 1) {
        tcp_port = atoi(argv[1]);
    }
    if (argc > 2) {
        udp_port = atoi(argv[2]);
    }
    init_client_sockets();

    init_console();
    printf("启动智能家具服务器...\n");
    init_console();
    if(network_init()<0) {
        fprintf(stderr, "网络初始化失败\n");
        return -1;
    }
    // 创建 TCP 和 UDP 套接字
    int success =0;
    tcp_sock = create_tcp_socket(&success);
    if(success<0)
        return -1;
    udp_sock = create_udp_socket(&success);
    if(success<0)
        return -1;

    // 设置 TCP 地址
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(tcp_port);

    // 绑定 TCP 套接字
    bind_socket(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr));

    // 监听 TCP 连接
    listen_socket(tcp_sock, MAX_CLIENTS);

    // 设置 UDP 地址
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(udp_port);

    // 绑定 UDP 套接字
    bind_socket(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr));

    printf("服务器启动成功!\n");
    printf("TCP服务端口: %d\n", TCP_PORT);
    printf("UDP服务端口: %d\n", UDP_PORT);



    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(tcp_sock, &read_fds);
        FD_SET(udp_sock, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1) {
                FD_SET(client_sockets[i], &read_fds);
            }
        }
        max_fd = (tcp_sock > udp_sock) ? tcp_sock : udp_sock;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1) {
                FD_SET(client_sockets[i], &read_fds);
                if (client_sockets[i] > max_fd) {
                    max_fd = client_sockets[i];
                }
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select错误");
            continue;
        }

        // 处理 TCP 连接请求
        if (FD_ISSET(tcp_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            my_socket client_sock = accept_connection(tcp_sock, (struct sockaddr*)&client_addr, &client_len);
            if (client_sock < 0) {
                perror("接受连接失败");
                continue;
            }

            // 查找空槽位并添加新客户端
            int empty_slot = find_empty_slot();
            if (empty_slot == -1) {
                printf("客户端连接已满，拒绝连接\n");
                close_socket(client_sock); // 拒绝连接
            }
            else {
                client_sockets[empty_slot] = client_sock;
                client_sensor_data[empty_slot] = (sensor_data_t*)malloc(sizeof(sensor_data_t) * MAX_SENSOR_DATA);  // 假设最多有 MAX_SENSOR_DATA 个传感器数据
                if (client_sensor_data[empty_slot] == NULL) {
                    perror("内存分配失败");
                    close_socket(client_sock);
                    client_sockets[empty_slot] = -1;
                }
                else {
                    printf("新的TCP客户端连接: %s:%d\n",
                            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                }

            }
        }

        // 处理 UDP 数据
        if (FD_ISSET(udp_sock, &read_fds)) {
            handle_udp_data(udp_sock);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != -1 && FD_ISSET(client_sockets[i], &read_fds)) {
                if (handle_tcp_connection(client_sockets[i])<0) {
                    remove_client(i);
                }
            }
        }
    }

    close_socket(tcp_sock);
    close_socket(udp_sock);
    network_cleanup();
    return 0;
}

