#include "network_utils.h" // 引入头文件

#define TCP_PORT 8080
#define UDP_PORT 8081
#define MAX_CLIENTS 10
#define MAX_DEVICES 20
#define MAX_SENSOR_DATA 10

my_socket client_sockets[MAX_CLIENTS];
char client_ids[MAX_CLIENTS][4];


sensor_data_t* device_sensor_data[MAX_DEVICES];
char device_ids[MAX_DEVICES][4];
int data_counters[MAX_DEVICES];

my_socket tcp_sock, udp_sock;

void init() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
        client_ids[i][0] = '\0';
    }
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_ids[i][0] = '\0';
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

int getSocketIndex(my_socket client_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            return i;
        }
    }
    return -1;
}

int getSocketIndexByID(char* device_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ( strcmp(client_ids[i], device_id) == 0) {
            return i;
        }
    }
    return -1;
}

int getDeviceIndex(char* device_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(device_ids[i], device_id) == 0) {
            return i;
        }
    }
    return -1;
}

char* get_device_id_list() {
    static char result[BUFFER_SIZE];  // 结果字符串，确保可以存储拼接后的所有数据
    memset(result, 0, sizeof(result));  // 清空 result 字符串

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_ids[i][0] != '\0') {
            if (strlen(result) + strlen(device_ids[i]) + 2 < sizeof(result)) {
                strcat(result, device_ids[i]);  // 添加 client_id
                strcat(result, "\n");  // 添加换行符
            } else {
                printf("警告: 结果字符串空间不足，部分数据未拼接\n");
                break;
            }
        }
    }
    return result;  // 返回拼接后的结果字符串
}

void remove_client(int index) {
    if (index >= 0 && index < MAX_CLIENTS) {
        close_socket(client_sockets[index]);
        client_sockets[index] = -1;  // 清除套接字
        data_counters[index] = 0;
        printf("客户端套接字 %d 连接已断开\n", index);
    }
}

// 查找空设备槽位
int find_empty_device_slot() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_ids[i][0] == '\0') {
            return i;
        }
    }
    return -1;  // 如果没有空槽，返回 -1
}

void add_data(char* device_id,sensor_data_t sensor_data) {
    // 查找设备是否已经存在
    int device_index = getDeviceIndex(device_id);

    if (device_index >= 0) {
        // 设备已存在，检查该设备是否有空余位置
        if (data_counters[device_index] < MAX_CLIENTS) {
            // 设备已有空位，添加数据
            device_sensor_data[device_index][data_counters[device_index]] = sensor_data;
            data_counters[device_index]++;
            printf("设备 %s 数据已添加\n", device_id);
        } else {
            // 设备满，打印存储不足
            printf("警告: 设备 %s 存储不足，无法添加更多数据\n", device_id);
        }
    } else {
        // 设备不存在，尝试创建新设备
        int empty_slot = find_empty_device_slot();
        if (empty_slot != -1) {
            // 如果设备未满，创建新设备并添加数据
            strcpy(device_ids[empty_slot], device_id);
            device_sensor_data[empty_slot] = (sensor_data_t*)malloc(MAX_CLIENTS * sizeof(sensor_data_t));
            if (device_sensor_data[empty_slot] != NULL) {
                device_sensor_data[empty_slot][0] = sensor_data;
                data_counters[empty_slot] = 1;
                printf("设备 %s 已创建并添加数据\n", device_id);
            } else {
                printf("错误: 内存分配失败，无法创建设备\n");
            }
        } else {
            // 设备列表已满，无法添加新设备
            printf("警告: 设备列表已满，无法添加设备 %s\n", device_id);
        }
    }
}

// 处理 TCP 连接
int handle_tcp_connection(my_socket client_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv_data(client_sock, buffer, BUFFER_SIZE, 0);

    if (bytes_received <= 0) {
        return -1; // 如果接收失败或连接关闭，返回
    }

    device_data_t device_data;
    memcpy(&device_data, buffer, sizeof(device_data_t));
    char* client_id = device_data.device_id;

    server_message_t server_msg;
    // 检查数据类型，只处理 type == 2，即控制命令
    if (device_data.type == 2) {
        control_cmd_t cmd = device_data.data.control_cmd;
        if (client_id != NULL) {
            int index = getSocketIndex(client_sock);
            if (index >= 0)
                strcpy(client_ids[index], client_id);
            switch (cmd.cmd) {
                case 0: // 关闭连接
                {
                    printf("执行关闭: %s\n", cmd.param);

                    server_msg.type = MSG_TYPE_COMMAND_RESULT;
                    snprintf(server_msg.data.command_result, sizeof(server_msg.data.command_result), "连接断开");

                    // 发送数据
                    send_data(client_sock, &server_msg, sizeof(server_message_t), 0);
                    return -1; // 关闭连接，返回 -1
                    break;
                }
                case CMD_LIST_DEVICE: // 获取传感器列表
                {
                    printf("客户端请求传感器数据\n");

                    char* dev_list = get_device_id_list();
                    server_msg.type = MSG_TYPE_DEVICE_LIST_RESPONSE;
                    strncpy(server_msg.data.device_list, dev_list, sizeof(server_msg.data.device_list) - 1);
                    server_msg.data.device_list[sizeof(server_msg.data.device_list) - 1] = '\0';

                    // 发送数据
                    send_data(client_sock, &server_msg, sizeof(server_message_t), 0);
                    printf("传感器数据已发送\n");
                    break;
                }

                case CMD_LIST_DEVICE_DATA: // 设备数据
                {
                    int dev_index = getDeviceIndex(cmd.param);

                    server_msg.type = MSG_TYPE_SENSOR_DATA_RESPONSE;
                    int count = data_counters[dev_index];
                    if (count>0)
                        server_msg.data.sensor_data = device_sensor_data[dev_index][count-1];

                    // 发送数据
                    send_data(client_sock, &server_msg, sizeof(server_message_t), 0);
                    break;
                }
                case CMD_EXEC_DEVICE:
                {
                    server_msg.type = MSG_TYPE_COMMAND_RESULT;
                    snprintf(server_msg.data.command_result, sizeof(server_msg.data.command_result),
                               "执行控制命令%s成功", cmd.param);
                    send_data(client_sock, &server_msg, sizeof(server_message_t), 0);
                    break;
                }
                default:
                    printf("未知命令\n");
                    break;
            }
        }
    } else {
        printf("收到非控制命令类型的数据，忽略\n");
    }

    return 1;
}

int handle_udp_data(my_socket udp_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // 接收数据
    int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
        (struct sockaddr*)&client_addr, &addr_len);

    if (bytes_received <= 0) {
        return -1; // 如果接收失败或连接关闭，返回
    }

    device_data_t device_data;
    memcpy(&device_data, buffer, sizeof(device_data_t));
    char* device_id = device_data.device_id;

    int index = getSocketIndexByID(device_id);

    if (device_data.type == 1) {  // 传感器数据
        sensor_data_t sensor_data = device_data.data.sensor_data;
        add_data(device_id, sensor_data);
        printf("收到传感器数据 - 类型: %d, 数值: %.2f, 时间: %s\n",
               sensor_data.type, sensor_data.value, sensor_data.timestamp);

        // 报警检查
        if (index >= 0 && sensor_data.type == 1 && sensor_data.value > 30.0) {
            printf("警告: 温度过高! 当前温度: %.2f\n", sensor_data.value);
            alarm_data_t alarm_data;
            snprintf(alarm_data.device_id, sizeof(alarm_data.device_id),  device_id);
            snprintf(alarm_data.alarm_info, sizeof(alarm_data.alarm_info), "温度过高! 当前温度: %.2f", sensor_data.value);
            snprintf(alarm_data.timestamp, sizeof(alarm_data.timestamp), "%s", sensor_data.timestamp);
            server_message_t message ;
            message.type = MSG_TYPE_ALARM;
            message.data.alarm_data = alarm_data;

            send_data(client_sockets[index], &message, sizeof(message), 0);
        }
        return 1;
    }else {
        return -1;
    }
}

int main(int argc, char* argv[]) {
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
    init();
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
            }else {
                // 查找空槽位并添加新客户端
                int empty_slot = find_empty_slot();
                if (empty_slot == -1) {
                    printf("客户端连接已满，拒绝连接\n");
                    close_socket(client_sock); // 拒绝连接
                }
                else {
                    client_sockets[empty_slot] = client_sock;
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

