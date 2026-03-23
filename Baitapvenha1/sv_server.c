#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    int port = atoi(argv[1]);
    char *log_file = argv[2];

    int server_sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sk == -1) {
        perror("socket failed\n");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(port);

    if (bind(server_sk, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("bind() failed\n");
        return 1;
    }

    if (listen(server_sk, 10)) {
        perror("listen() failed\n");
        return 1;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char buffer[BUFFER_SIZE];

        printf("Server is listening on port %d...\n", port);
        
        int client_sk = accept(server_sk, (struct sockaddr *)&client_addr, &client_len);
        if (client_sk == -1) {
            perror("accept() failed\n");
            continue;
        }

        printf("There is a new client connected\n");

        FILE *f_log = fopen(log_file, "a"); 
        if (f_log) {
            while (1) {
                int bytes_received = recv(client_sk, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_received <= 0) {
                    break; 
                }
                
                time_t t = time(NULL);
                struct tm *now = localtime(&t);
                buffer[bytes_received] = '\0';
                fprintf(f_log, "%s %02d-%02d-%02d %02d:%02d:%02d %s\n", inet_ntoa(client_addr.sin_addr), now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour + 7, now->tm_min, now->tm_sec, buffer);
                fflush(f_log); 
                printf("Received from client: %s\n", buffer);
            }
            fclose(f_log);
        }

        close(client_sk);
    }

    close(server_sk);
    return 0;
}