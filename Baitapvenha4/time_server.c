/*******************************************************************************
 * @file    multiprocess_server.c
 * @brief   Mô tả ngắn gọn về chức năng của file
 * @date    2026-05-05 08:30
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

void signal_handler(int sig) {
    int pid = wait(NULL);
    printf("Child process terminated: %d\n", pid);
}

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
    
    // Server is now listening for incoming connections
    printf("Server is listening on port 7000...\n");
    
    signal(SIGCHLD, signal_handler);

    while (1) {
        int client = accept(listener, NULL, NULL);
        printf("New client connected: %d\n", client);
        if (fork() == 0) {
            // Xu ly trong tien trinh con
            close(listener);

            char buf[1024];
            while (1) {
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) break;

                buf[ret] = '\0';
                if (buf[ret-1] == '\n') buf[ret-1] = '\0';
                if (buf[ret-2] == '\r') buf[ret-2] = '\0';

                char cmd[20], format[50];
                int n = sscanf(buf, "%s %s", cmd, format);

                if (n == 2 && strcmp(cmd, "GET_TIME") == 0) {
                    time_t rawtime;
                    struct tm *timeinfo;
                    char buffer[80];

                    time(&rawtime);
                    timeinfo = localtime(&rawtime);

                    if (strcmp(format, "dd/mm/yyyy") == 0) {
                        strftime(buffer, sizeof(buffer), "%d/%m/%Y\n", timeinfo);
                    } else if (strcmp(format, "dd/mm/yy") == 0) {
                        strftime(buffer, sizeof(buffer), "%d/%m/%y\n", timeinfo);
                    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
                        strftime(buffer, sizeof(buffer), "%m/%d/%Y\n", timeinfo);
                    } else if (strcmp(format, "mm/dd/yy") == 0) {
                        strftime(buffer, sizeof(buffer), "%m/%d/%y\n", timeinfo);
                    } else {
                        strcpy(buffer, "Dinh dang khong hop le!\n");
                    }

                    send(client, buffer, strlen(buffer), 0);
                } else {
                    char *error_msg = "Sai cu phap; cu phap mau: GET_TIME format\n";
                    send(client, error_msg, strlen(error_msg), 0);
                }
            }
            close(client);
            exit(0);
        }
        // Xu ly o tien trinh cha
        close(client);
    }

    close(listener);
    return 0;
}