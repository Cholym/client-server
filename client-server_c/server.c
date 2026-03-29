#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define PORT 2255
#define STUDENT_ID "5239"
#define MAX_LINE 256
#define RESPONSE_SIZE 8192

// 1.	Создайте серверное приложение:
// a.	Создаётся сокет, порт 2255.
// b.	Сообщение «etc» → строка, начинающаяся с «server%STUDENT_ID%», и список через запятую имён *.conf в /etc.
// c.	Сообщение «dev» → строка со списком через запятую имён каталогов в /dev.

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static int send_all(int fd, const char *data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = send(fd, data + sent, size - sent, 0);
        if (n < 0) {
            // Если вызов был прерван сигналом, не считаем его ошибкой и продолжаем отправку
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            return -1;
        sent += (size_t)n;
    }
    return 0;
}

// fd - дескриптор сокета
// cap - максимальный размер буфера
static int read_line(int fd, char *line, size_t cap) {
    size_t i = 0;
    if (cap == 0)
        return -1;
    while (i < cap - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            if (i == 0)
                return -1;
            break;
        }
        if (c == '\n')
            break;
        // для windows файлов
        if (c != '\r')
            line[i++] = c;
    }
    line[i] = '\0';
    return 0;
}

static int ends_with(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len)
        return 0;
    return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

static int is_regular_file(const char *dir, const char *name) {
    char path[PATH_MAX];
    struct stat st;
    if (snprintf(path, sizeof(path), "%s/%s", dir, name) >= (int)sizeof(path))
        return 0;
    if (stat(path, &st) != 0)
        return 0;
    return S_ISREG(st.st_mode); // проверяем, является ли файл обычным файлом
}

static int is_directory(const char *dir, const char *name) {
    char path[PATH_MAX];
    struct stat st;
    if (snprintf(path, sizeof(path), "%s/%s", dir, name) >= (int)sizeof(path))
        return 0;
    if (stat(path, &st) != 0)
        return 0;
    return S_ISDIR(st.st_mode);
}

static int append_name_csv(char *out, size_t out_size, size_t *pos, int *first, const char *name) {
    int n;
    if (*first)
        *first = 0;
    else {
        n = snprintf(out + *pos, out_size - *pos, ",");
        if (n < 0 || (size_t)n >= out_size - *pos)
            return -1;
        *pos += (size_t)n;
    }
    n = snprintf(out + *pos, out_size - *pos, "%s", name);
    if (n < 0 || (size_t)n >= out_size - *pos)
        return -1;
    *pos += (size_t)n;
    return 0;
}

static int build_etc_conf_list(char *out, size_t out_size) {
    int n = snprintf(out, out_size, "server%s,", STUDENT_ID);
    if (n < 0 || (size_t)n >= out_size)
        return -1;
    size_t pos = (size_t)n;
    int first = 1;

    DIR *dir = opendir("/etc");
    if (dir == NULL)
        return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (!ends_with(entry->d_name, ".conf"))
            continue;
        if (!is_regular_file("/etc", entry->d_name))
            continue;
        if (append_name_csv(out, out_size, &pos, &first, entry->d_name) != 0) {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return 0;
}

static int build_dev_dir_list(char *out, size_t out_size) {
    out[0] = '\0';
    size_t pos = 0;
    int first = 1;

    DIR *dir = opendir("/dev");
    if (dir == NULL)
        return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (!is_directory("/dev", entry->d_name))
            continue;

        if (append_name_csv(out, out_size, &pos, &first, entry->d_name) != 0) {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return 0;
}

int main() {
    int server_fd;  // просто число
    int new_socket; // просто число
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char line[MAX_LINE];
    char response[RESPONSE_SIZE];

    // игнорируем сигнал SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // назначается дескриптор
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // настройка сокета
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

    if (listen(server_fd, 8) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;
        }

        while (read_line(new_socket, line, sizeof(line)) == 0) {
            trim_newline(line);
            if (strcmp(line, "etc") == 0) {
                if (build_etc_conf_list(response, sizeof(response)) != 0)
                    snprintf(response, sizeof(response), "server%s,error", STUDENT_ID);
            } else if (strcmp(line, "dev") == 0) {
                if (build_dev_dir_list(response, sizeof(response)) != 0)
                    snprintf(response, sizeof(response), "error");
            } else
                snprintf(response, sizeof(response), "Unknown command");

            size_t len = strlen(response);
            if (len + 2 < sizeof(response)) {
                response[len] = '\n';
                response[len + 1] = '\0';
                len++;
            }
            if (send_all(new_socket, response, len) != 0)
                break;
        }
        close(new_socket);
    }
}
