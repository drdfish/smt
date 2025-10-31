#include "network_utils.h" // ����ͷ�ļ�

#define TCP_PORT 8080
#define UDP_PORT 8081

// ���� TCP ����
int handle_tcp_connection(my_socket client_sock) {
    char buffer[BUFFER_SIZE];
    control_cmd_t cmd;

    int bytes_received = recv_data(client_sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) return -1;

    memcpy(&cmd, buffer, sizeof(cmd));

    switch (cmd.cmd)
    {
    case 1: // ��ȡ����������
    {
        printf("�ͻ������󴫸�������\n");

        // ģ�⴫��������
        sensor_data_t sensor_data = { 1, 25.5, "2025-10-24 10:00:00" };

        send_data(client_sock, &sensor_data, sizeof(sensor_data), 0);
        break;
    }

    case 2: // �����豸
    {
        printf("ִ�п�������: %s\n", cmd.param);

        char response[] = "��������ִ�гɹ�";

        send_data(client_sock, response, strlen(response), 0);
        break;
    }

    default:
    {
        printf("δ֪����\n");
        break;
    }
    }

    return 0;
}

// ���� UDP ����
int handle_udp_data(my_socket udp_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
        (struct sockaddr*)&client_addr, &addr_len);

    if (bytes_received > 0) {
        sensor_data_t sensor_data;
        memcpy(&sensor_data, buffer, sizeof(sensor_data));

        printf("�յ����������� - ����:%d ��ֵ:%.2f ʱ��:%s\n",
            sensor_data.type, sensor_data.value, sensor_data.timestamp);

        // ���ݴ洢�߼�
        FILE* log_file = fopen("sensor_log.txt", "a");
        if (log_file) {
            fprintf(log_file, "����:%d, ��ֵ:%.2f, ʱ��:%s\n",
                sensor_data.type, sensor_data.value, sensor_data.timestamp);
            fclose(log_file);
        }

        // �������
        if (sensor_data.type == 1 && sensor_data.value > 30.0) {
            printf("����: �¶ȹ���! ��ǰ�¶�: %.2f\n", sensor_data.value);
        }
    }

    return 0;
}

int main() {
    my_socket tcp_sock, udp_sock;
    struct sockaddr_in tcp_addr, udp_addr;
    fd_set read_fds;
    int max_fd;

    printf("�������ܼҾӻ�����ط�����...\n");
    if(network_init()<0)
        return -1;
    // ���� TCP �� UDP �׽���
    tcp_sock = create_tcp_socket();
    udp_sock = create_udp_socket();

    // ���� TCP ��ַ
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(TCP_PORT);

    // �� TCP �׽���
    bind_socket(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr));

    // ���� TCP ����
    listen_socket(tcp_sock, MAX_CLIENTS);

    // ���� UDP ��ַ
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(UDP_PORT);

    // �� UDP �׽���
    bind_socket(udp_sock, (struct sockaddr*)&udp_addr, sizeof(udp_addr));

    printf("�����������ɹ�!\n");
    printf("TCP����˿�: %d\n", TCP_PORT);
    printf("UDP����˿�: %d\n", UDP_PORT);

    max_fd = (tcp_sock > udp_sock) ? tcp_sock : udp_sock;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(tcp_sock, &read_fds);
        FD_SET(udp_sock, &read_fds);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select����");
            continue;
        }

        // ���� TCP ��������
        if (FD_ISSET(tcp_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_sock = accept_connection(tcp_sock, (struct sockaddr*)&client_addr, &client_len);
            if (client_sock < 0) {
                perror("��������ʧ��");
                continue;
            }

            printf("�µ�TCP�ͻ�������: %s:%d\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            handle_tcp_connection(client_sock);
            close_socket(client_sock);
        }

        // ���� UDP ����
        if (FD_ISSET(udp_sock, &read_fds)) {
            handle_udp_data(udp_sock);
        }
    }

    close_socket(tcp_sock);
    close_socket(udp_sock);
    network_cleanup();
    return 0;
}

