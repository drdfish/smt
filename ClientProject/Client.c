// client.c
#include "network_utils.h"

#define SERVER_IP "127.0.0.1"
#define TCP_PORT 8080
#define UDP_PORT 8081

#define DEVICE_ID "DEV0"


// 新增：处理从服务器收到的消息
int handle_server_message(my_socket sock) {
    server_message_t response;
    int bytes_received = recv_data(sock, &response, sizeof(response), 0);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("\n[系统] 服务器已关闭连接。\n");
        } else {
            perror("[系统] 接收数据失败");
        }
        // 标记需要退出程序
        return -1;
    }

    // 根据消息类型处理
    switch (response.type) {
        case MSG_TYPE_ALARM:
            // 收到告警信息，需要特殊处理以避免打断用户输入
            // \r 会将光标移到行首
            printf("\r\n--- 收到服务器告警 ---\n");
            printf("  设备ID: %s\n", response.data.alarm_data.device_id);
            printf("  告警信息: %s\n", response.data.alarm_data.alarm_info);
            printf("  时间戳: %s\n", response.data.alarm_data.timestamp);
            printf("----------------------\n");
            printf(">> "); // 重新打印提示符
            fflush(stdout); // 确保提示符立即显示
            break;

        case MSG_TYPE_SENSOR_DATA_RESPONSE:
            printf("\n--- 收到传感器数据 ---\n");
            printf("  数据类型: %d\n", response.data.sensor_data.type);
            printf("  数    值: %.2f\n", response.data.sensor_data.value);
            printf("  时 间 戳: %s\n", response.data.sensor_data.timestamp);
            printf("------------------------\n");
            break;

        case MSG_TYPE_COMMAND_RESULT:
            printf("\n--- 服务器执行结果 ---\n%s\n--------------------------\n", response.data.command_result);
            break;

        case MSG_TYPE_DEVICE_LIST_RESPONSE:
            printf("\n--- 设备列表 ---\n%s\n----------------\n", response.data.device_list);
            break;

        case MSG_TYPE_CONFIG_RESPONSE:
            printf("\n--- 配置结果 ---\n%s\n----------------\n", response.data.config_response);
            break;

        default:
            printf("\n[警告] 收到未知类型的服务器消息: %d\n", response.type);
            break;
    }
    return 0;
}

// 新增：解析并处理用户输入的命令
void process_user_command(char* cmd_line, my_socket sock) {
    char *command;
    char *arg1, *arg2;

    // 使用strtok分割命令
    command = strtok(cmd_line, " \n"); // 按空格或换行符分割
    if (command == NULL) {
        return; // 用户只输入了回车
    }

    if (strcmp(command, "help") == 0) {
        printf("可用命令:\n");
        printf("  ls                  : 列出所有在线设备\n");
        printf("  ls -s <device_id>   : 查询指定设备的传感器数据\n");
        printf("  conf <device_id>    : 配置指定设备参数\n");
        printf("  exec <device_id>    : 在指定设备上执行远程命令\n");
        printf("  exit                : 退出客户端\n");
    } else if (strcmp(command, "ls") == 0) {
        arg1 = strtok(NULL, " \n");
        if (arg1 != NULL && strcmp(arg1, "-s") == 0) {
            // 这是 'ls -s <device_id>' 命令
            arg2 = strtok(NULL, " \n");
            if (arg2 == NULL) {
                printf("[错误] 用法: ls -s <device_id>\n");
                return;
            }
            printf("正在查询设备 '%s' 的传感器数据...\n", arg2);
            // TODO: 构建并发送获取传感器数据的命令到服务器
            // 这里需要根据你的具体协议来构建 device_data_t
        } else {
            // 这是 'ls' 命令
            printf("正在请求设备列表...\n");
            // TODO: 构建并发送列出设备的命令到服务器
        }
    } else if (strcmp(command, "conf") == 0) {
        arg1 = strtok(NULL, " \n");
        if (arg1 == NULL) {
            printf("[错误] 用法: conf <device_id>\n");
            return;
        }
        printf("准备配置设备 '%s' (此功能待实现)...\n", arg1);
        // TODO: 实现配置逻辑，可能需要更多用户输入
    } else if (strcmp(command, "exec") == 0) {
         // TODO: 实现远程执行命令逻辑
        printf("远程执行命令功能待实现。\n");
    }
     else if (strcmp(command, "exit") == 0) {
        printf("正在断开连接...\n");
        close_socket(sock);
        exit(0);
    } else {
        printf("[错误] 未知命令: '%s'。输入 'help' 查看帮助。\n", command);
    }
}

// 用户输入处理线程
void* user_input_thread(void* arg) {
    my_socket sock = *(my_socket*)arg;
    char user_input_buffer[BUFFER_SIZE];

    while (1) {
        printf(">> ");
        fflush(stdout);

        if (fgets(user_input_buffer, sizeof(user_input_buffer), stdin) != NULL) {
            process_user_command(user_input_buffer, sock);
        } else {
            break;
        }
    }
    return NULL;
}



// TCP控制客户端，用于向服务器发送控制命令并接收响应。
void tcp_control_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    fd_set read_fds; // 用于select的读文件描述符集合
    int max_fd;      // select需要监视的最大文件描述符

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

    printf("连接到TCP服务器 %s:%d 成功\n", SERVER_IP, TCP_PORT);
    printf("输入 'help' 查看可用命令。\n");
    printf("------------------------------------\n");

    // 创建用户输入线程
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)user_input_thread, &sock, 0, NULL);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, user_input_thread, &sock);
#endif

    // 主线程只处理网络消息
    while (1) {
        if(handle_server_message(sock) < 0) break;
    }

    // 清理线程
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
#else
    pthread_join(thread, NULL);
#endif

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
        device_data_t device_data = {

        };
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