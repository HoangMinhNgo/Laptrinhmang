#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];

    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    int res = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res == -1) {
        printf("Khong ket noi duoc den server! ");
        return 1;
    }
    
    printf("Connected to server\n");
    char greeting[1024];
    int len = recv(client_socket, greeting, sizeof(greeting) - 1, 0); 
    greeting[len] = 0;
    printf("%s\n", greeting);

    while (1) {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        if (strncmp(buffer, "exit", 4) == 0) break;

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            printf("send() failed\n");
            break;
        }
    }

    close(client_socket);

    return 0;
}