#include "network_utils.h"

#define SERVER_IP "127.0.0.1"
#define TCP_PORT 8080
#define UDP_PORT 8081

void tcp_control_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    printf("����TCP���ƿͻ���...\n");

    int success = 0;
    sock = create_tcp_socket(&success);
    if (success < 0) {
        perror("����TCP�׽���ʧ��");
        exit(1);
    }
        
    // ���ӷ�����
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP����ʧ��");
        exit(1);
    }

    printf("���ӵ�TCP�������ɹ�\n");

    control_cmd_t cmd = { 1, "��ȡ�¶�����" };
    send_data(sock, &cmd, sizeof(cmd), 0);

    sensor_data_t sensor_data;
    int bytes_received = recv_data(sock, &sensor_data, sizeof(sensor_data), 0);

    if (bytes_received > 0) {
        printf("�յ�����������:\n");
        printf("  ����: %s\n", sensor_data.type == 1 ? "�¶�" : "����");
        printf("  ��ֵ: %.2f\n", sensor_data.value);
        printf("  ʱ��: %s\n", sensor_data.timestamp);
    }

    close_socket(sock);
}

void udp_sensor_client() {
    my_socket sock;
    struct sockaddr_in server_addr;

    printf("����UDP�������ͻ���...\n");

    int success = 0;
    sock = create_udp_socket(&success);
    if (success < 0) {
        perror("����TCP�׽���ʧ��");
        exit(1);
    }
    

    for (int i = 0; i < 5; i++) {
        sensor_data_t sensor_data;
        char timestamp[20];

        get_current_time(timestamp);

        sensor_data.type = 1; // �¶ȴ�����
        sensor_data.value = 25.0 + (rand() % 100) / 10.0;
        strcpy(sensor_data.timestamp, timestamp);

        send_data_with_addr(sock, &sensor_data, sizeof(sensor_data), 0,
            (struct sockaddr_in*)&server_addr, sizeof(server_addr));

        printf("���ʹ���������: ����=%d, ��ֵ=%.2f, ʱ��=%s\n",
            sensor_data.type, sensor_data.value, sensor_data.timestamp);

        platform_sleep(2);
    }

    close_socket(sock);
}

int main(int argc, char* argv[]) {
    printf("���ܼҾӼ�ؿͻ�������\n");

    if (argc > 1 && strcmp(argv[1], "control") == 0) {
        tcp_control_client();
    }
    else {
        udp_sensor_client();
    }

    return 0;
}