#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Определяем размер буфера
#define BUFFER_SIZE 1024

// Функция для обработки ошибок
void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    // Проверяем количество аргументов командной строки
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    // Получаем IP сервера
    char *server_ip = argv[1];
    // Получаем порт сервера
    int server_port = atoi(argv[2]);

    // Создаем сокет сервера
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        // Обрабатываем ошибку создания сокета
        error("Error opening socket");
    }

    // Настраиваем адрес сервера
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

    // Привязываем сокет к порту
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        // Обрабатываем ошибку привязки сокета к порту
        error("Error on binding");
    }

    // Начинаем прослушивание порта на наличие входящих подключений
    listen(server_socket, 3);

    printf("Server is waiting for clients...\n");

    // Принимаем подключения клиентов
    int client_sockets[3];
    int connected_clients = 0;
    while (connected_clients < 3) {
        client_sockets[connected_clients] = accept(server_socket, NULL, NULL);
        if (client_sockets[connected_clients] < 0) {
            // Обрабатываем ошибку приема подключений
            error("Error on accept");
        }

        printf("Client connected: %d\n", connected_clients);
        connected_clients++;
    }

    // Инициализируем информацию об игроках
    int energies[3]; // Энергия каждого игрока
    int is_alive[3]; // Жив ли игрок
    int resting_player; // Игрок, который будет отдыхать
    int has_rest_event_occurred = 0; // Флаг события отдыха
    memset(energies, 0, sizeof(energies));
    memset(is_alive, 1, sizeof(is_alive));

    // Выбираем игрока, который будет отдыхать
    resting_player = rand() % 3;

    // Отправляем начальные сообщения клиентам
    char buffer[BUFFER_SIZE];
    srand(time(NULL));
    for (int i = 0; i < 3; i++) {
        energies[i] = rand() % 100 + 1;
        if (i == resting_player) {
            snprintf(buffer, BUFFER_SIZE,
                     "You are player %d. Your initial energy is %d."
                     " You will rest and double your energy during the first battle.\n",
                     i, energies[i]);
        } else {
            snprintf(buffer, BUFFER_SIZE, "You are player %d."
                                          " Your initial energy is %d.\n", i, energies[i]);
        }
        write(client_sockets[i], buffer, strlen(buffer));
    }

    // Цикл игры
    int round = 0; // Переменная для отслеживания раундов
    while (1) {
        // Игрок, который отдыхает, удваивает свою энергию, если событие отдыха еще не произошло, и это первый раунд
        if (!has_rest_event_occurred && round == 0) {
            energies[resting_player] *= 2;

            // Отправляем сообщение об отдыхе игроку
            snprintf(buffer, BUFFER_SIZE, "You are resting. Your energy is doubled to %d.\n", energies[resting_player]);
            write(client_sockets[resting_player], buffer, strlen(buffer));

            has_rest_event_occurred = 1; // Событие отдыха произошло
        }

        // Выбираем двух случайных игроков для боя
        int player1 = -1;
        int player2 = -1;
        while (player1 == -1 || player2 == -1) {
            player1 = rand() % 3;
            player2 = rand() % 3;
            if (player1 == player2 || !is_alive[player1] || !is_alive[player2]) {
                player1 = -1;
                player2 = -1;
            }
        }

        // Отправляем сообщение о бое игрокам
        snprintf(buffer, BUFFER_SIZE, "Battle: Player %d vs Player %d\n", player1, player2);
        write(client_sockets[player1], buffer, strlen(buffer));
        write(client_sockets[player2], buffer, strlen(buffer));

        // Проводим бой
        if (energies[player1] > energies[player2]) {
            energies[player1] += energies[player2];
            is_alive[player2] = 0;

            snprintf(buffer, BUFFER_SIZE, "You won the battle against Player %d!\n", player2);
            write(client_sockets[player1], buffer, strlen(buffer));
            snprintf(buffer, BUFFER_SIZE, "You lost the battle against Player %d.\n", player1);
            write(client_sockets[player2], buffer, strlen(buffer));
        } else {
            energies[player2] += energies[player1];
            is_alive[player1] = 0;

            snprintf(buffer, BUFFER_SIZE, "You won the battle against Player %d!\n", player1);
            write(client_sockets[player2], buffer, strlen(buffer));
            snprintf(buffer, BUFFER_SIZE, "You lost the battle against Player %d.\n", player2);
            write(client_sockets[player1], buffer, strlen(buffer));
        }

        // Проверяем если остлся один игрок
        int remaining_player = -1;
        for (int i = 0; i < 3; i++) {
            if (is_alive[i]) {
                if (remaining_player == -1) {
                    remaining_player = i;
                } else {
                    remaining_player = -1;
                    break;
                }
            }
        }

        // Если остался один игрок -  завершаем игру
        if (remaining_player != -1) {
            snprintf(buffer, BUFFER_SIZE, "Congratulations! You are the last survivor. Your energy is %d.\n",
                     energies[remaining_player]);
            write(client_sockets[remaining_player], buffer, strlen(buffer));
            break;
        }

        round++; // Счетчик рраундов
    }

    // Закрытие клиентов
    for (int i = 0; i < 3; i++) {
        close(client_sockets[i]);
    }
    close(server_socket);

    return 0;
}
