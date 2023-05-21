#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Server IP> <Server Port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        // Ошибка при открытии сокета
        error("Error opening socket");
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    if (connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        // Ошибка при подключении к серверу
        error("Error connecting to server");
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t read_size;
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        read_size = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (read_size < 0) {
            // Ошибка при чтении данных от сервера
            error("Error reading from server");
        }

        if (strncmp(buffer, "You lost", 8) == 0) {
            // Выводим сообщение "You lost"
            printf("%s", buffer);
            break;
        }
        // Выводим полученные данные от сервера
        printf("%s", buffer);
    }

    // Закрываем клиентский сокет
    close(client_socket);

    return 0;
}
