#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define PORT 2255
#define STUDENT_ID "5239"
#define MAX_LINE 256
#define RESPONSE_SIZE 8192

// 2.	Создайте клиентское приложение:
// a.	Подключитесь к сокету сервера, отправьте строку “etc” и выведите полученную строку на экран.
// b.	Подключитесь к сокету сервера, отправьте строку “dev” и выведите полученную строку на экран.

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_LINE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    /* Сервер читает строку до '\n' (read_line) — без \n будет взаимная блокировка. */
    const char *cmd_etc = "etc\n";
    if (send(sockfd, cmd_etc, strlen(cmd_etc), 0) < 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    printf("Received: %s", buffer);

    const char *cmd_dev = "dev\n";
    if (send(sockfd, cmd_dev, strlen(cmd_dev), 0) < 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    printf("Received: %s", buffer);

    close(sockfd);
    return 0;
}