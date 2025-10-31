// client.c
#include "network_utils.h"

#define SERVER_IP "127.0.0.1"
#define TCP_PORT 8080
#define UDP_PORT 8081


// TCP控制客户端，用于向服务器发送控制命令并接收响应。
void tcp_control_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    printf("启动TCP控制客户端...\n");

    // 1. 创建套接字
    int success = 0;
    sock = create_tcp_socket(&success);
    if (success < 0) {
        perror("TCP Error");
        exit(1);
    }

    // 2. 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);

    // 将字符串IP地址转换为网络格式
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("无效地址或地址族不被支持");
        close_socket(sock);
        return;
    }

    // 3. 连接服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP连接失败");
        close_socket(sock);
        return;
    }

    printf("连接到TCP服务器 %s:%d 成功\n", SERVER_IP, TCP_PORT);
    printf("------------------------------------\n");

    // 4. 主命令循环
    while (1) {
        printf("\n请输入命令:\n");
        printf("  1: 获取传感器数据\n");
        printf("  2: 让服务器执行远程命令\n");
        printf("  0: 退出\n");
        printf(">> ");

        char input_buffer[16];
        int choice = -1;

        // 读取用户输入
        if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
            // 解析整数输入
            sscanf(input_buffer, "%d", &choice);
        } else {
            // 读取失败 (例如用户输入了文件结束符 Ctrl+D)
            printf("输入流结束，正在退出...\n");
            break;
        }

        if (choice == 0) {
            printf("正在断开连接...\n");
            break; // 跳出循环以关闭客户端
        }

        if (choice == 1) {
            // 准备发送“获取数据”命令
            control_cmd_t cmd;
            cmd.cmd = 1; // 命令类型
            strncpy(cmd.param, "get_data", sizeof(cmd.param) - 1);
            cmd.param[sizeof(cmd.param) - 1] = '\0'; // 确保字符串以空字符结尾

            // 发送命令
            if (send_data(sock, &cmd, sizeof(cmd), 0) < 0) {
                 perror("发送命令失败");
                 break; // 发送失败，很可能连接已断开，退出循环
            }

            // 接收服务器返回的传感器数据
            sensor_data_t sensor_data;
            int bytes_received = recv_data(sock, &sensor_data, sizeof(sensor_data), 0);

            if (bytes_received > 0) {
                // 成功接收到数据
                printf("\n--- 收到服务器响应 ---\n");
                printf("  数据类型: %d\n", sensor_data.type);
                printf("  数    值: %.2f\n", sensor_data.value);
                printf("  时 间 戳: %s\n", sensor_data.timestamp);
                printf("------------------------\n");
            } else if (bytes_received == 0) {
                // 服务器主动关闭了连接
                printf("服务器已关闭连接。\n");
                break;
            } else {
                // 接收数据时发生错误
                perror("接收数据失败");
                break;
            }
        } else if (choice == 2) {
            // ----- 新增：执行远程命令 -----
            control_cmd_t cmd;
            cmd.cmd = 2; // 命令类型 2 代表执行远程命令

            char command_input[256]; // 用于读取用户输入的缓冲区
            printf("请输入要远程执行的命令 (最大 %zu 字符): ", sizeof(cmd.param) - 1);

            // 读取一整行命令
            if (fgets(command_input, sizeof(command_input), stdin) == NULL) {
                printf("读取命令失败。\n");
                continue; // 返回菜单
            }

            // 移除 fgets 读取到的末尾换行符
            command_input[strcspn(command_input, "\n")] = 0;

            // 将命令复制到结构体中
            strncpy(cmd.param, command_input, sizeof(cmd.param) - 1);
            cmd.param[sizeof(cmd.param) - 1] = '\0';

            // 发送命令
            if (send_data(sock, &cmd, sizeof(cmd), 0) < 0) {
                perror("发送执行命令失败");
                break;
            }

            // 准备接收服务器返回的执行结果
            char response_buffer[BUFFER_SIZE];
            int bytes_received = recv_data(sock, response_buffer, sizeof(response_buffer) - 1, 0);

            if (bytes_received > 0) {
                response_buffer[bytes_received] = '\0'; // 确保字符串正确结尾
                printf("\n--- 服务器执行结果 ---\n%s\n--------------------------\n", response_buffer);
            } else if (bytes_received == 0) {
                printf("服务器已关闭连接。\n");
                break;
            } else {
                perror("接收执行结果失败");
                break;
            }
        }
        else {
            printf("无效的命令 '%d'，请重新输入。\n", choice);
        }
    } // 循环结束

    // 5. 关闭套接字
    close_socket(sock);
    printf("TCP客户端已关闭。\n");
}

//UDP传感器客户端，模拟传感器设备，定期向服务器上报数据。
void udp_sensor_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    printf("启动UDP传感器客户端...\n");

    int success = 0;
    sock = create_udp_socket(&success);

    if (success < 0) {
        perror("TCP Error");
        exit(1);
    }

    srand((unsigned int)time(NULL)); // 初始化随机数种子

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("无效地址或地址族不被支持");
        exit(EXIT_FAILURE);
    }

    // 循环发送5次模拟数据
    for (int i = 0; i < 5; i++) {
        sensor_data_t sensor_data;
        char timestamp[20];

        get_current_time(timestamp);

        sensor_data.type = 1; // 模拟温度传感器
        sensor_data.value = 25.0 + (rand() % 100) / 10.0; // 生成 25.0 ~ 34.9 之间的随机温度
        strcpy(sensor_data.timestamp, timestamp);

        // 发送数据到服务器
        send_data_with_addr(sock, &sensor_data, sizeof(sensor_data), 0,
            &server_addr, sizeof(server_addr));

        printf("发送传感器数据: 类型=%d, 数值=%.2f, 时间=%s\n",
            sensor_data.type, sensor_data.value, sensor_data.timestamp);

        platform_sleep(2); // 跨平台延时
    }

    close_socket(sock);
    printf("UDP客户端已关闭。\n");
}

//主函数，根据命令行参数选择启动TCP或UDP客户端。
int main(int argc, char* argv[]) {
    init_console();
    // 初始化网络库 (在Windows上此操作是必需的)
    if (network_init() < 0) {
        fprintf(stderr, "网络初始化失败\n");
        return 1;
    }

    printf("智能家居监控客户端启动\n");

    // 如果命令行参数包含 "control"，则启动TCP控制客户端
    // 否则，默认启动UDP传感器客户端
    if (argc > 1 && strcmp(argv[1], "control") == 0) {
        tcp_control_client();
    } else {
        tcp_control_client();
        // udp_sensor_client();
    }

    // 清理网络库 (在Windows上此操作是必需的)
    network_cleanup();

    return 0;
}