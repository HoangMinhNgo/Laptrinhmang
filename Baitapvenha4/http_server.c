#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#define NUM_PROCESSES 5

void signal_handler(int sig) {
    int pid = wait(NULL);
    printf("Child process terminated: %d\n", pid);
}

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
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (fork() == 0) {
            while (1) {
                int client = accept(listener, NULL, NULL);
                if (client < 0) continue;

                char buf[1024];
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    close(client);
                    continue;
                }
                buf[ret] = 0;
                printf("--- Request received by Process %d ---\n%s\n", getpid(), buf);

                char *msg = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: 45\r\n"
                            "Connection: close\r\n\r\n"
                            "<html><body><h1>Xin chao cac ban!</h1></body></html>";
                
                send(client, msg, strlen(msg), 0);
                
                close(client);
            }
        }
    }


    signal(SIGCHLD, signal_handler);
    wait(NULL);

    close(listener);
    return 0;
}