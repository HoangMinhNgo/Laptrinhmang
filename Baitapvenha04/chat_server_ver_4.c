#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char id[50];
    int is_authenticated;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(const char *message, int sender_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_sock && clients[i].is_authenticated == 1) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void direct_message(const char *message, const char *receiver, int sender_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].id, receiver) == 0 &&clients[i].socket != sender_sock && clients[i].is_authenticated == 1) {
            send(clients[i].socket, message, strlen(message), 0);
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == sock) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_thread(void *);

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
    
    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;
        printf("New client connected: %d\n", client);

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            clients[client_count].socket = client;
            clients[client_count].is_authenticated = 0;
            memset(clients[client_count].id, 0, 50);
            client_count++;

            int *new_sock = malloc(sizeof(int));
            *new_sock = client;

            pthread_t tid;
            if (pthread_create(&tid, NULL, client_thread, (void*)new_sock) == 0) {
                pthread_detach(tid); 
            } else {
                free(new_sock);
                close(client);
            }
        } else {
            char *full_msg = "ERROR Server hien tai da day phong chat!\r\n";
            send(client, full_msg, strlen(full_msg), 0);
            close(client);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(listener);
    return 0;
}

void *client_thread(void *params) {
    int my_sock = *(int*)params;
    free(params);

    char buffer[BUFFER_SIZE];
    char client_id[50];
    int read_size;
    int authenticated = 0;

    char *welcome = "Hay dang nhap theo cu phap 'client_id: client_name'\r\n";
    send(my_sock, welcome, strlen(welcome), 0);

    while (!authenticated) {
        memset(buffer, 0, BUFFER_SIZE);
        read_size = recv(my_sock, buffer, BUFFER_SIZE - 1, 0);
        if (read_size <= 0) {
            close(my_sock);
            remove_client(my_sock);
            return NULL; 
        }

        while (read_size > 0 && (buffer[read_size - 1] == '\n' || buffer[read_size - 1] == '\r')) {
            buffer[read_size - 1] = '\0';
            read_size--;
        }

        char cmd[32], tmp[32];
        int n = sscanf(buffer, "%s%s%s", cmd, client_id, tmp);
        if (n != 2) {
            char *msg = "Error. Thua hoac thieu tham so!\n";
            send(my_sock, msg, strlen(msg), 0);
        } else {
            if (strcmp(cmd, "client_id:") != 0) {
                char *msg = "Error. Sai cu phap!\n";
                send(my_sock, msg, strlen(msg), 0);
            } else {
                //ids[i] = malloc(strlen(id) + 1);
                //strcpy(ids[i], id);
                authenticated = 1;

                pthread_mutex_lock(&clients_mutex);
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].socket == my_sock) {
                        strcpy(clients[i].id, client_id);
                        clients[i].is_authenticated = 1;
                        break;
                    }
                }
                pthread_mutex_unlock(&clients_mutex);
                char success_msg[128];
                snprintf(success_msg, sizeof(success_msg), "SUCCESS Dang nhap thanh cong voi ID: %s\r\n", client_id);
                send(my_sock, success_msg, strlen(success_msg), 0);
                printf("[Server] Client tai socket %d da dang ky ID: %s\n", my_sock, client_id);
            }
        }

        if (!authenticated) {
            
        }
    }

    while ((read_size = recv(my_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        while (read_size > 0 && (buffer[read_size - 1] == '\n' || buffer[read_size - 1] == '\r')) {
            buffer[read_size - 1] = '\0';
            read_size--;
        }
        if (strlen(buffer) == 0) continue;


        char target[32];
        int n = sscanf(buffer, "%s", target);
        if (n == 0) 
            continue;
        
        if (strcmp(target, "all") == 0) {
            // Chuyen tiep tin nhan cho tat ca client
            char *pos = buffer + strlen(target) + 1;
            char broadcast_buf[BUFFER_SIZE + 256];
            snprintf(broadcast_buf, sizeof(broadcast_buf), "%s: %s\r\n", client_id, pos);
            broadcast_message(broadcast_buf, my_sock);
        } else {
            char *pos = buffer + strlen(target) + 1;
            char broadcast_buf[BUFFER_SIZE + 256];
            snprintf(broadcast_buf, sizeof(broadcast_buf), "%s: %s\r\n", client_id, pos);
            direct_message(broadcast_buf, target, my_sock);
        }                                 
    }
    printf("[Server] Client %s (Socket %d) da ngat ket noi.\n", client_id, my_sock);
    close(my_sock);
    remove_client(my_sock);

    return NULL;
}