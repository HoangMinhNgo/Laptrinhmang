/*******************************************************************************
 * @file    multiprocess_server.c
 * @brief   Mô tả ngắn gọn về chức năng của file
 * @date    2026-05-05 08:30
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h> 

void signal_handler(int sig) {
    int pid = wait(NULL);
    //printf("Child process terminated: %d\n", pid);
}

void handle_calc_logic(int client_fd) {
    char raw_data[4096] = {0};
    recv(client_fd, raw_data, sizeof(raw_data) - 1, 0);

    char http_method[16], path_info[256];
    sscanf(raw_data, "%s %s", http_method, path_info);

    char operator[16] = "";
    double val1 = 0, val2 = 0;
    int is_valid = 0;

    if (strcmp(http_method, "GET") == 0) {
        char *q_ptr = strchr(path_info, '?');
        if (q_ptr && sscanf(q_ptr + 1, "op=%[^&]&a=%lf&b=%lf", operator, &val1, &val2) == 3)
            is_valid = 1;
    } 
    else if (strcmp(http_method, "POST") == 0) {
        char *body_ptr = strstr(raw_data, "\r\n\r\n");
        if (body_ptr && sscanf(body_ptr + 4, "op=%[^&]&a=%lf&b=%lf", operator, &val1, &val2) == 3)
            is_valid = 1;
    }

    char html_output[1024];
    if (is_valid) {
        double calc_res = 0;
        char symbol = '?';
        if (strcmp(operator, "add") == 0) { calc_res = val1 + val2; symbol = '+'; }
        else if (strcmp(operator, "sub") == 0) { calc_res = val1 - val2; symbol = '-'; }
        else if (strcmp(operator, "mul") == 0) { calc_res = val1 * val2; symbol = '*'; }
        else if (strcmp(operator, "div") == 0 && val2 != 0) { calc_res = val1 / val2; symbol = '/'; }
        
        snprintf(html_output, sizeof(html_output), "<html><body><h1>Ket qua: %.2g %c %.2g = %.2g</h1></body></html>", val1, symbol, val2, calc_res);
    } else {
        snprintf(html_output, sizeof(html_output), "<html><body><h2>Thong bao: Sai cu phap. Format: ?op=add&a=10&b=5</h2></body></html>");
    }

    char final_response[2048];
    snprintf(final_response, sizeof(final_response), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n%s", html_output);
    send(client_fd, final_response, strlen(final_response), 0);
    close(client_fd);
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
    
    signal(SIGCHLD, signal_handler);

    while (1) {
        int client = accept(listener, NULL, NULL);
        //printf("New client connected: %d\n", client);
        if (fork() == 0) {
            // Xu ly trong tien trinh con
            close(listener);
            handle_calc_logic(client);
            exit(0);
        }
        // Xu ly o tien trinh cha
        close(client);
    }

    close(listener);
    return 0;
}