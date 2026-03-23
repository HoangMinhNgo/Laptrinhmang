#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char mssv[10], hoTen[50], ngaySinh[20], diemTrungBinh[5];

    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == -1) {
        printf("socket() failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    int res = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == -1){
        perror("connect() failed\n");
        close(client_socket);
        exit(1);
    }

    printf("Connected to server\n");
    buffer[0] = '\0';

    printf("Nhập MSSV: ");
    fgets(mssv, sizeof(mssv), stdin);
    mssv[strcspn(mssv, "\n")] = ' '; 
    strcat(buffer, mssv);
    printf("Nhập Họ Tên: ");
    fgets(hoTen, sizeof(hoTen), stdin);
    hoTen[strcspn(hoTen, "\n")] = ' '; 
    strcat(buffer, hoTen);
    printf("Nhập Ngày Sinh: ");
    fgets(ngaySinh, sizeof(ngaySinh), stdin);
    ngaySinh[strcspn(ngaySinh, "\n")] = ' '; 
    strcat(buffer, ngaySinh);
    printf("Nhập Điểm Trung Bình: ");
    fgets(diemTrungBinh, sizeof(diemTrungBinh), stdin);
    strcat(buffer, diemTrungBinh);

    send(client_socket, buffer, strlen(buffer), 0);

    close(client_socket);
    return 0;
}
