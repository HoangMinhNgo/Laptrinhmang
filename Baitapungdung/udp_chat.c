#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s port_s ip_d port_d\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port_s  = atoi(argv[1]);
    char *ip_d  = argv[2];
    int port_d  = atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port_s);

    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port   = htons(port_d);
    if (inet_pton(AF_INET, ip_d, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid destination IP: %s\n", ip_d);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) { perror("fcntl F_GETFL"); close(sockfd); exit(EXIT_FAILURE); }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

    printf("UDP Chat started — listening on port %d, sending to %s:%d\n",
           port_s, ip_d, port_d);
    printf("Type a message and press Enter to send. Press Ctrl+C to quit.\n\n");

    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];
    int  send_pos = 0; 

    while (1) {

        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n == 1) {
            if (ch == '\n') {
                send_buf[send_pos] = '\0';
                if (send_pos > 0) {
                    ssize_t sent = sendto(sockfd, send_buf, send_pos, 0,
                                         (struct sockaddr *)&dest_addr,
                                         sizeof(dest_addr));
                    if (sent < 0)
                        perror("sendto");
                    else
                        printf("[Sent %zd bytes] > %s\n", sent, send_buf);
                    send_pos = 0;
                }
            } else {
                if (send_pos < BUFFER_SIZE - 1)
                    send_buf[send_pos++] = ch;
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read stdin");
        }

        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        ssize_t recv_n = recvfrom(sockfd, recv_buf, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&sender_addr, &sender_len);
        if (recv_n > 0) {
            recv_buf[recv_n] = '\0';
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, sizeof(sender_ip));
            printf("\n[Recv from %s:%d] %s\n",
                   sender_ip, ntohs(sender_addr.sin_port), recv_buf);
        } else if (recv_n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom");
        }
    }

    close(sockfd);
    return 0;
}