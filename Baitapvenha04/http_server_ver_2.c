#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

void *client_thread(void *);
int listener;

int main() {
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    
    pthread_t threads[10];
    for (int i = 0; i < 10; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        
        if (pthread_create(&threads[i], NULL, client_thread, thread_id) != 0) {
            perror("Khong the tao thread");
            return 1;
        }
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }

    close(listener);
    return 0;
}

void *client_thread(void *params) {
    int thread_id = *(int*)params;
    free(params);
    
    char buf[256];
    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html><body><h1>Hi, nice to meet u!</h1></body></html>";

    while (1) {
        int client = accept(listener, NULL, NULL);
        
        if (client < 0) {
            perror("Loi accept");
            sleep(1); 
            continue;
        }

        printf("Thread %d xu ly client socket: %d\n", thread_id, client);

        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0) {
            buf[ret] = 0;
            send(client, msg, strlen(msg), 0);
        }
        close(client); 
    }
    return NULL;
}