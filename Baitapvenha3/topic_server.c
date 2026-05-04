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
    int topicnum;
    char topic[100][25];
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
    addr.sin_port = htons(9000);
    
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
    printf("Server is listening on port 9000...\n");

    struct ClientInfo clients[MAX_CLIENTS];
    char buf[256];
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    while (1) {

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

                clients[nfds].topicnum = 0;

                char *msg = "Xin chao. Ban co the SUB hoac PUB cac topic!\n";
                for(int i = 0; i < 100; ++i)
                {
                    memset(clients[nfds].topic[i], 0, sizeof(clients[nfds].topic[i]));
                }
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
                    char cmd[32],topic[32];
                    int n = sscanf(buf, "%s%s", cmd, topic);
                    if (n == 0) 
                        continue;
                    if (strcmp(cmd, "SUB") == 0) {
                        char *msg = "Ban da dang ky topic thanh cong!\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                            
                            //ids[i] = malloc(strlen(id) + 1);
                            //strcpy(ids[i], id);
                        
                        strcpy(clients[i].topic[clients[i].topicnum], topic);
                        clients[i].topicnum++;
                    } else if(strcmp(cmd, "UNSUB") == 0){
                        int j = 0;
                        while(strcmp(clients[i].topic[j], topic) != 0){
                            j++;
                            if(j >= clients[i].topicnum)
                            {
                                break;
                            }
                        }
                        if(j == clients[i].topicnum){
                            char *msg = "Ban chua dang ky topic muon bo dang ky!\n";
                            send(fds[i].fd, msg, strlen(msg), 0);
                        } else{
                            if(clients[i].topicnum == 1)
                            {
                                char *msg = "UNSUB topic thanh cong!\n";
                                send(fds[i].fd, msg, strlen(msg), 0);
                                memset(clients[i].topic[0], 0, sizeof(clients[i].topic[0]));
                                clients[i].topicnum--;
                            }
                            else{
                                char *msg = "UNSUB topic thanh cong!\n";
                                send(fds[i].fd, msg, strlen(msg), 0);
                                for(int t = j; t < clients[i].topicnum - 1; ++t) {
                                    strcpy(clients[i].topic[t], clients[i].topic[t + 1]);
                                }
                                clients[i].topicnum--;
                            }
                        }
                    } else if(strcmp(cmd, "PUB") == 0) {
                        for (int j = 1; j < nfds; j++) {

                            for(int t = 0; t < clients[j].topicnum; ++t)
                            {
                                if(strcmp(clients[j].topic[t], topic) == 0)
                                {
                                    send(fds[j].fd, topic, strlen(topic), 0);
                                    send(fds[j].fd, ": ", 2, 0);
                                    char *pos = buf + strlen(cmd) + strlen(topic) + 2;
                                    send(fds[j].fd, pos, strlen(pos), 0);
                                    break;
                                }
                            }

                        }
                    } else {
                        char *msg = "Ban nhap sai cu phap!\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                    }                                  
                }     
            }
        }
    }

    close(listener);
    return 0;
}