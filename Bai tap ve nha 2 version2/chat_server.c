/*******************************************************************************
 * @file    chat_server.c
 * @brief   Yeu cau client dang nhap va chuyen tiep tin nhan.
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
    char id[64];
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
    char buf[256];
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    while (1) {
        // Reset tap fdtest
        //fdtest = fdread;

        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll() failed");
            break;
        }

        if (fds[0].revents && POLLIN) {
            int client = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS) {
                printf("New client connected: %d\n", client);

                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;

                clients[nfds].state = 0;
                memset(clients[nfds].id, 0, sizeof(clients[nfds].id));

                char *msg = "Xin chao. Hay dang nhap!\n";
                send(client, msg, strlen(msg), 0);
                nfds++;
            } else {
                close(client);
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents && POLLIN) {
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
                        printf("Received from %d: %s\n", i, buf);

                        // Xu ly dang nhap
                        if (clients[i].state == 0) {
                            // Chua dang nhap
                            // Kiem tra cu phap "client_id: client_name"
                            char cmd[32], id[32], tmp[32];
                            int n = sscanf(buf, "%s%s%s", cmd, id, tmp);
                            if (n != 2) {
                                char *msg = "Error. Thua hoac thieu tham so!\n";
                                send(fds[i].fd, msg, strlen(msg), 0);
                            } else {
                                if (strcmp(cmd, "client_id:") != 0) {
                                    char *msg = "Error. Sai cu phap!\n";
                                    send(fds[i].fd, msg, strlen(msg), 0);
                                } else {
                                    char *msg = "OK. Hay nhap tin nhan!\n";
                                    send(fds[i].fd, msg, strlen(msg), 0);
                                    
                                    //ids[i] = malloc(strlen(id) + 1);
                                    //strcpy(ids[i], id);
                                    clients[i].state = 1;
                                    strcpy(clients[i].id, id);
                                }
                            }
                        } else {
                            // Da dang nhap
                            char target[32];
                            int n = sscanf(buf, "%s", target);
                            if (n == 0) 
                                continue;
                            
                            if (strcmp(target, "all") == 0) {
                                // Chuyen tiep tin nhan cho tat ca client
                                for (int j = 1; j < nfds; j++)
                                    if (clients[j].state != 0 && i != j) {
                                        send(fds[j].fd, clients[i].id, strlen(clients[i].id), 0);
                                        send(fds[j].fd, ": ", 2, 0);
                                        char *pos = buf + strlen(target) + 1;
                                        send(fds[j].fd, pos, strlen(pos), 0);
                                    }
                            } else {
                                int j = 1;
                                for (;j < nfds; j++)
                                    if (clients[j].id != 0 && strcmp(target, clients[j].id) == 0)
                                        break;
                                if (j < nfds) {
                                    // Tim thay
                                    send(fds[j].fd, clients[i].id, strlen(clients[i].id), 0);
                                    send(fds[j].fd, ": ", 2, 0);
                                    char *pos = buf + strlen(target) + 1;
                                    send(fds[j].fd, pos, strlen(pos), 0);
                                } else {
                                    // Khong tim thay
                                    continue;
                                }
                            }                                  
                        }
                    }
                           
            }
        }
    }

    close(listener);
    return 0;
}