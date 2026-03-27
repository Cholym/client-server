#include <netinet/in.h> // Структуры для Internet-адресов (struct sockaddr_in)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 2255
#define STUDENT_ID 5239
#define RESPONSE_SIZE 8192

// 1.	Создайте серверное приложение:
// a.	Создайте сокет, используйте номер порта 2255.
// b.	Дождитесь сообщения “etc” от клиента, верните строку, начинающуюся со строки “server%STUDENT_ID%“ и содержащую разделенный запятыми список всех файлов *.conf в каталоге /etc.
// c.	Дождитесь сообщения “dev” от клиента, верните строку, содержащую разделенный запятыми список всех каталогов в каталоге /dev.

void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

int build_csv_from_cmd(const char *cmd, char *out, size_t out_size, const char *prefix) {
    FILE *fp = popen(cmd, "r");
    if (prefix != NULL) {
        strncpy(out, prefix, out_size - 1);
        out[out_size - 1] = '\0';
    }
    if (fp == NULL) {
        return -1;
    }
    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_newline(line);
        if (strlen(out) + strlen(line) + 2 > out_size) {
            pclose(fp);
            return -1;
        }
        strcat(out, ",");
        strcat(out, line);
    }
    pclose(fp);
    return 0;
}


int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Создаем сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }



    close(new_socket);
    close(server_fd);
    return 0;
}