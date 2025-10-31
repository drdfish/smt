#include "network_utils.h"

#define SERVER_IP "127.0.0.1"
#define TCP_PORT 8080
#define UDP_PORT 8081

void tcp_control_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    printf("启动TCP控制客户端...\n");

    int success = 0;
    sock = create_tcp_socket(&success);
    if (success < 0) {
        perror("创建TCP套接字失败");
        exit(1);
    }
        
    // 连接服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP连接失败");
        exit(1);
    }

    printf("连接到TCP服务器成功\n");

    control_cmd_t cmd = { 1, "获取温度数据" };
    send_data(sock, &cmd, sizeof(cmd), 0);

    sensor_data_t sensor_data;
    int bytes_received = recv_data(sock, &sensor_data, sizeof(sensor_data), 0);

    if (bytes_received > 0) {
        printf("收到传感器数据:\n");
        printf("  类型: %s\n", sensor_data.type == 1 ? "温度" : "其他");
        printf("  数值: %.2f\n", sensor_data.value);
        printf("  时间: %s\n", sensor_data.timestamp);
    }

    close_socket(sock);
}

void udp_sensor_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    printf("启动UDP传感器客户端...\n");

    int success = 0;
    sock = create_udp_socket(&success);
    if (success < 0) {
        perror("创建TCP套接字失败");
        exit(1);
    }
    

    for (int i = 0; i < 5; i++) {
        sensor_data_t sensor_data;
        char timestamp[20];

        get_current_time(timestamp);

        sensor_data.type = 1; // 温度传感器
        sensor_data.value = 25.0 + (rand() % 100) / 10.0;
        strcpy(sensor_data.timestamp, timestamp);

        send_data_with_addr(sock, &sensor_data, sizeof(sensor_data), 0,
            (struct sockaddr_in*)&server_addr, sizeof(server_addr));

        printf("发送传感器数据: 类型=%d, 数值=%.2f, 时间=%s\n",
            sensor_data.type, sensor_data.value, sensor_data.timestamp);

        platform_sleep(2);
    }

    close_socket(sock);
}

int main(int argc, char* argv[]) {
    printf("智能家居监控客户端启动\n");

    if (argc > 1 && strcmp(argv[1], "control") == 0) {
        tcp_control_client();
    }
    else {
        udp_sensor_client();
    }

    return 0;
}