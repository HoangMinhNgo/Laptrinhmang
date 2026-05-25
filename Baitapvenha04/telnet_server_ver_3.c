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
#define DB_FILE "users.txt"

int check_login(const char *username, const char *password) {
    FILE *file = fopen(DB_FILE, "r");
    if (file == NULL) {
        printf("[Lỗi] Không thể mở file cơ sở dữ liệu %s\n", DB_FILE);
        return 0;
    }

    char line[256];
    char db_user[128], db_pass[128];
    int authenticated = 0;

    while (fgets(line, sizeof(line), file) != NULL) {

        if (sscanf(line, "%s %s", db_user, db_pass) == 2) {
            if (strcmp(username, db_user) == 0 && strcmp(password, db_pass) == 0) {
                authenticated = 1;
                break;
            }
        }
    }

    fclose(file);
    return authenticated;
}

void send_file_content(int client_sock, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        char *err = "ERROR Không thể đọc kết quả lệnh\r\n";
        send(client_sock, err, strlen(err), 0);
        return;
    }

    char file_buf[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), file)) > 0) {
        send(client_sock, file_buf, bytes_read, 0);
    }

    fclose(file);
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
    int read_size;
    int logged_in = 0;

    char out_filename[64];
    snprintf(out_filename, sizeof(out_filename), "out_%d.txt", my_sock);

    char *welcome = "Dang nhap tai khoan user pass:\r\n";
    send(my_sock, welcome, strlen(welcome), 0);

    while (!logged_in) {
        memset(buffer, 0, BUFFER_SIZE);
        read_size = recv(my_sock, buffer, BUFFER_SIZE - 1, 0);
        if (read_size <= 0) {
            close(my_sock);
            return NULL; 
        }

        while (read_size > 0 && (buffer[read_size - 1] == '\n' || buffer[read_size - 1] == '\r')) {
            buffer[read_size - 1] = '\0';
            read_size--;
        }

        if (strlen(buffer) == 0) continue;

        char username[128] = {0};
        char password[128] = {0};

        if (sscanf(buffer, "%s %s", username, password) == 2) {
            if (check_login(username, password)) {
                logged_in = 1;
                char *success_msg = "Dang nhap thanh cong; hay nhap lenh can thuc thi:\r\n";
                send(my_sock, success_msg, strlen(success_msg), 0);
                printf("[Server] Socket %d dang nhap thanh cong bang tai khoan: %s\n", my_sock, username);
            }
        }

        if (!logged_in) {
            char *fail_msg = "Dang nhập that bai, thu lai user pass:\r\n";
            send(my_sock, fail_msg, strlen(fail_msg), 0);
        }
    }

    while ((read_size = recv(my_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        while (read_size > 0 && (buffer[read_size - 1] == '\n' || buffer[read_size - 1] == '\r')) {
            buffer[read_size - 1] = '\0';
            read_size--;
        }
        
        if (strlen(buffer) == 0) continue;
        

        printf("Socket %d thuc thi lenh: %s\n", my_sock, buffer);

        char system_cmd[BUFFER_SIZE + 128];
        snprintf(system_cmd, sizeof(system_cmd), "%s > %s 2>&1", buffer, out_filename); 

        system(system_cmd);

        send_file_content(my_sock, out_filename);
        
        send(my_sock, "\r\n", 2, 0);
    }

    printf("[Server] Socket %d da ngat ket noi.\n", my_sock);
    close(my_sock);
    unlink(out_filename);

    return NULL;
}