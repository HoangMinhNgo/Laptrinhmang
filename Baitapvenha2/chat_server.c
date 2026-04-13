#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define MAX_CLIENTS 1020
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char name[50];
    int is_registered;
} Client;

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(7000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    Client clients[MAX_CLIENTS] = {0};
    fd_set readfds;
    printf("Server đang chạy trên port 7000...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        int max_fd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd) max_fd = clients[i].fd;
            }
        }

        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ret == 0) {
            printf("Timed out\n");
            continue;
        }

        if (FD_ISSET(listener, &readfds)) {
            int client = accept(listener, NULL, NULL);
            int joined = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = client;
                    clients[i].is_registered = 0;
                    memset(clients[i].name, 0, 50);
                    char *msg = "Vui lòng nhập tên theo cú pháp 'client_id: name':\n";
                    send(client, msg, strlen(msg), 0);
                    joined = 1;
                    break;
                }
            }
            if (!joined) close(client);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i].fd;
            if (fd > 0 && FD_ISSET(fd, &readfds) > 0) {
                char buf[BUFFER_SIZE];
                memset(buf, 0, BUFFER_SIZE);
                int n = recv(fd, buf, sizeof(buf) - 1, 0);

                if (n <= 0) {
                    printf("Client %s đã thoát.\n", clients[i].name);
                    close(fd);
                    clients[i].fd = 0;
                    continue;
                }

                buf[strcspn(buf, "\r\n")] = 0;

                if (!clients[i].is_registered) {
                    char name_buf[50];
                    if (sscanf(buf, "client_id: %49[^\n]", name_buf) == 1) {
                        strncpy(clients[i].name, name_buf, 49);
                        clients[i].name[49] = '\0';
                        clients[i].is_registered = 1;
                        char *ok = "Đăng ký thành công! Bạn có thể bắt đầu chat.\n";
                        send(fd, ok, strlen(ok), 0);
                    } else {
                        char *err = "Sai cú pháp! Thử lại: 'client_id: name'\n";
                        send(fd, err, strlen(err), 0);
                    }
                } else {
                    time_t rawtime;
                    time(&rawtime);
                    struct tm *ti = localtime(&rawtime);
                    char time_str[30];
                    strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", ti);

                    char send_buf[BUFFER_SIZE + 150];
                    snprintf(send_buf, sizeof(send_buf), "%s %s: %s\n", time_str, clients[i].name, buf);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd > 0 && clients[j].is_registered) {
                            send(clients[j].fd, send_buf, strlen(send_buf), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}