#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#define BUFFER_SIZE 1024

int process_time_command(const char *cmd, char *output, size_t max_len) {
    if (strncmp(cmd, "GET_TIME ", 9) != 0) {
        return 0;
    }

    const char *format = cmd + 9;

    time_t raw_time = time(NULL);
    struct tm *time_info = localtime(&raw_time);

    if (strcmp(format, "dd/mm/yyyy") == 0) {
        strftime(output, max_len, "%d/%m/%Y\r\n", time_info);
        return 1;
    } else if (strcmp(format, "dd/mm/yy") == 0) {
        strftime(output, max_len, "%d/%m/%y\r\n", time_info);
        return 1;
    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
        strftime(output, max_len, "%m/%d/%Y\r\n", time_info);
        return 1;
    } else if (strcmp(format, "mm/dd/yy") == 0) {
        strftime(output, max_len, "%m/%d/%y\r\n", time_info);
        return 1;
    }

    return 0;
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

        int *new_sock = malloc(sizeof(int));
        *new_sock = client;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, (void*)new_sock) == 0) {
            pthread_detach(tid); 
        } else {
            free(new_sock);
            close(client);
        }
    }

    close(listener);
    return 0;
}

void *client_thread(void *params) {
    int my_sock = *(int*)params;
    free(params);

    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int read_size;

    printf("Ket noi voi client tai socket %d\n", my_sock);

    while ((read_size = recv(my_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        while (read_size > 0 && (buffer[read_size - 1] == '\n' || buffer[read_size - 1] == '\r')) {
            buffer[read_size - 1] = '\0';
            read_size--;
        }

        if (strlen(buffer) == 0) continue;

        printf("Nhan lenh tu socket %d: '%s'\n", my_sock, buffer);

        memset(response, 0, BUFFER_SIZE);
        if (process_time_command(buffer, response, BUFFER_SIZE)) {
            send(my_sock, response, strlen(response), 0);
        } else {
            char *err_msg = "ERROR Sai cu phap lenh hoac dinh dang!\r\n";
            send(my_sock, err_msg, strlen(err_msg), 0);
        }
    }

    printf("Client tai socket %d da ngat ket noi\n", my_sock);
    close(my_sock);
    return NULL;
}