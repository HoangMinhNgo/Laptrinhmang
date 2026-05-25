#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#define BUFFER_SIZE 1024

typedef struct {
    int client1;
    int client2;
} ChatPair;

typedef struct {
    int from_sock;
    int to_sock;
} ForwardArgs;

void *client_thread(void *);

void start_chat_session(int c1, int c2) {
    printf("[Server] Ghép đôi thành công: Socket %d <==> Socket %d\n", c1, c2);

    char* welcome_msg = "SUCCESS Bạn đã được kết nối với bạn chat! Hãy bắt đầu trò chuyện.\r\n";
    send(c1, welcome_msg, strlen(welcome_msg), 0);
    send(c2, welcome_msg, strlen(welcome_msg), 0);

    pthread_t thread1, thread2;

    ForwardArgs* args1 = malloc(sizeof(ForwardArgs));
    args1->from_sock = c1;
    args1->to_sock = c2;
    pthread_create(&thread1, NULL, client_thread, (void*)args1);
    pthread_detach(thread1); 

    ForwardArgs* args2 = malloc(sizeof(ForwardArgs));
    args2->from_sock = c2;
    args2->to_sock = c1;
    pthread_create(&thread2, NULL, client_thread, (void*)args2);
    pthread_detach(thread2);
}

int main() {
    int waiting_client = -1; 
    pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

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

        pthread_mutex_lock(&queue_mutex);

        if (waiting_client == -1) {
            waiting_client = client;
            char* wait_msg = "WAIT Đang tìm kiếm bạn chat, vui lòng đợi...\r\n";
            send(client, wait_msg, strlen(wait_msg), 0);
            printf("[Server] Socket %d đang ở hàng đợi.\n", client);
        } else {
            int client1 = waiting_client;
            int client2 = client;
            
            waiting_client = -1; 

            start_chat_session(client1, client2);
        }

        pthread_mutex_unlock(&queue_mutex);
    }

    close(listener);
    return 0;
}

void *client_thread(void *params) {
    ForwardArgs* f_args = (ForwardArgs*)params;
    int from = f_args->from_sock;
    int to = f_args->to_sock;
    free(f_args);

    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(from, buffer, sizeof(buffer), 0)) > 0) {
        send(to, buffer, bytes_received, 0);
    }

    printf("[Server] Một client đã ngắt kết nối. Đóng cả cặp chat...\n");
    
    close(from);
    close(to);

    return NULL;
}