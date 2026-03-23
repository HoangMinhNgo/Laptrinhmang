#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    int port = atoi(argv[1]);
    char *hello_file = argv[2];
    char *log_file = argv[3];

    int server_sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sk == -1) {
        printf("Failed to create socket\n");
        return 1;
    }

    int opt = 1;
    setsockopt(server_sk, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(port);

    if (bind(server_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed\n");
        exit(1);
    }

    if (listen(server_sk, 10) < 0) {
        perror("listen failed\n");
        exit(1);
    }

    while (1) {

        printf("server is listening on port %d...\n", port);

        int client_sk = accept(server_sk, NULL, NULL);
        if (client_sk == -1) {
            perror("accept() failed\n");
            continue;
        }

        printf("There is a new client connected\n");

        FILE *f_hello = fopen(hello_file, "r");
        char buffer[BUFFER_SIZE];
        if (f_hello) {
            if (fgets(buffer, sizeof(buffer), f_hello) != NULL) {
                send(client_sk, buffer, strlen(buffer), 0);
            }
            fclose(f_hello);
        } else {
            printf("Failed to open hello file\n");
        }

        FILE *f_log = fopen(log_file, "a"); 
        if (f_log) {
            while (1) {
                int bytes_received = recv(client_sk, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_received <= 0) {
                    break; 
                }
                
                buffer[bytes_received] = '\0';
                fprintf(f_log, "%s", buffer);
                fflush(f_log); 
            }
            fclose(f_log);
        }

        close(client_sk);
    }

    close(server_sk);
    return 0;
}