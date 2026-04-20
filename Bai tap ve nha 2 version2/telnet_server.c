/*******************************************************************************
 * @file    telnet_server.c
 * @brief   Client dang nhap va thuc hien lenh
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#define MAX_CLIENTS 1000

struct ClientInfo {
    int state;
};

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
    
    struct ClientInfo clients[MAX_CLIENTS];
    struct pollfd fds[MAX_CLIENTS];

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    int nfds = 1;

    char buf[256];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll() failed");
            break;
        }

        // Kiem tra socket listener
        if (fds[0].revents && POLLIN) {
            int client = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS) {
                printf("New client connected: %d\n", client);

                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;

                nfds++;
            } else {
                close(client);
            }
        }

        for (int i = 1; i < nfds; i++) {
            if(fds[i].revents && POLLIN){
                    ret = recv(fds[i].fd, buf, sizeof(buf), 0);
                    if (ret <= 0) {
                        printf("Client %d disconnected\n", i);    
                        close(fds[i].fd);
                        fds[i] = fds[nfds - 1];
                        clients[i] = clients[nfds - 1];
                        nfds--;
                        i--;
                    } else {
                        buf[ret] = 0;
                        if (buf[strlen(buf) - 1] == '\n')
                            buf[strlen(buf) - 1] = 0;
                        printf("Received from %d: %s\n", i, buf);

                        if (clients[i].state == 0) {
                            // Kiem tra ky tu enter
                            char user[32], pass[32], tmp[64];
                            int n = sscanf(buf, "%s%s%s", user, pass, tmp);
                            if (n != 2) {
                                char *msg = "Sai cu phap. Hay dang nhap lai.\n";
                                send(fds[i].fd, msg, strlen(msg), 0);
                            } else {
                                sprintf(tmp, "%s %s", user, pass);
                                int found = 0;
                                char line[64];
                                FILE *f = fopen("users.txt", "r");
                                while (fgets(line, sizeof(line), f) != NULL) {
                                    if (line[strlen(line) - 1] == '\n')
                                        line[strlen(line) - 1] = 0;
                                    if (strcmp(line, tmp) == 0) {
                                        found = 1;
                                        break;
                                    }
                                }
                                fclose(f);

                                if (found == 1) {
                                    char *msg = "OK. Hay nhap lenh.\n";
                                    send(fds[i].fd, msg, strlen(msg), 0);
                                    clients[i].state = 1;
                                } else {
                                    char *msg = "Sai username hoac password. Hay dang nhap lai.\n";
                                    send(fds[i].fd, msg, strlen(msg), 0);
                                }
                            }
                        } else {
                            // Thuc hien lenh tu client
                            char cmd[512];
                            sprintf(cmd, "%s > out.txt", buf);
                            system(cmd);
                            FILE *f = fopen("out.txt", "rb");
                            while (1) {
                                int len = fread(buf, 1, sizeof(buf), f);
                                if (len <= 0)
                                    break;
                                send(fds[i].fd, buf, len, 0);
                            }
                            fclose(f);
                        }
                    }
                }         
            }
    }

    close(listener);
    return 0;
}