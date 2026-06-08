/*******************************************************************************
 * @file    multiprocess_server.c
 * @brief   Mô tả ngắn gọn về chức năng của file
 * @date    2026-05-05 08:30
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

void signal_handler(int sig) {
    int pid = wait(NULL);
    //printf("Child process terminated: %d\n", pid);
}

void clean_url(char *dest, const char *src) {
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            *dest++ = ' '; src += 3;
        } else if (*src == '+') {
            *dest++ = ' '; src++;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}

const char* get_content_type(const char *path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".jpg") || strstr(path, ".png")) return "image/jpeg";
    if (strstr(path, ".mp4")) return "video/mp4";
    if (strstr(path, ".mp3")) return "audio/mpeg";
    return "text/plain";
}

void stream_file(int client, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        char *err = "HTTP/1.1 404 Not Found\r\n\r\nFile Not Found";
        send(client, err, strlen(err), 0);
        return;
    }
    
    char head[256];
    snprintf(head, sizeof(head), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", get_content_type(path));
    send(client, head, strlen(head), 0);
    
    char chunk[8192];
    size_t len;
    while ((len = fread(chunk, 1, sizeof(chunk), fp)) > 0) send(client, chunk, len, 0);
    fclose(fp);
}

void list_directory(int client, const char *dir_path, const char *uri) {
    DIR *d = opendir(dir_path);
    if (!d) return;

    send(client, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>", 55, 0);
    
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        
        char full[1024], link[1024];
        snprintf(full, sizeof(full), "%s/%s", dir_path, entry->d_name);
        snprintf(link, sizeof(link), "%s%s%s", uri, (strcmp(uri, "/") == 0 ? "" : "/"), entry->d_name);
        
        struct stat st;
        stat(full, &st);
        
        if (S_ISDIR(st.st_mode))
            dprintf(client, "<b><a href='%s'>%s/</a></b><br>", link, entry->d_name);
        else
            dprintf(client, "<i><a href='%s'>%s</a></i><br>", link, entry->d_name);
    }
    send(client, "</body></html>", 14, 0);
    closedir(d);
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
        if (fork() == 0) {
            // Xu ly trong tien trinh con
            close(listener);

            char req[2048], method[16], uri[256], path[512] = ".";
            recv(client, req, sizeof(req), 0);
            sscanf(req, "%s %s", method, uri);
            
            clean_url(path + 1, uri);
            
            struct stat st;
            if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
                list_directory(client, path, uri);
            else
                stream_file(client, path);
            close(client);
            exit(0);
        }
        // Xu ly o tien trinh cha
        close(client);
    }

    close(listener);
    return 0;
}