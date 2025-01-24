/**
 * @file server.c
 * @brief Implementação do servidor do jogo de labirinto.
 *
 * Este arquivo contém a implementação do servidor do jogo de labirinto,
 * responsável por gerenciar as conexões com os clientes, processar os comandos
 * recebidos e enviar as respostas apropriadas. O servidor utiliza sockets para
 * comunicação em rede e um algoritmo de busca em largura (BFS) para gerar dicas
 * de caminho.
 */
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

// Tamanho máximo do buffer de mensagens
#define BUFSZ 1024
// Tamanho máximo do tabuleiro
#define MAX_BOARD_SIZE 10
// Tamanho mínimo do tabuleiro
#define MIN_BOARD_SIZE 5
// Tamanho máximo do caminho para dicas
#define MAX_PATH 100

// Variáveis globais do jogo
int board_size;
int game_board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
int discovered[MAX_BOARD_SIZE][MAX_BOARD_SIZE] = {0};

int player_x = 0;
int player_y = 0;
int game_started = 0;
int show_full_map = 0;
int game_completed = 0;

// Constantes para os elementos do mapa
#define WALL 0         // Parede
#define PATH 1         // Caminho livre
#define ENTRANCE 2     // Entrada do labirinto
#define EXIT 3         // Saída do labirinto
#define UNDISCOVERED 4 // Célula não descoberta
#define PLAYER 5       // Posição do jogador

// Nome do arquivo do mapa
#define MAP_FILE "input/in.txt"

/**
 * @brief Exibe a mensagem de uso do programa e encerra a execução.
 *
 * @param argc Número de argumentos da linha de comando.
 * @param argv Vetor de strings contendo os argumentos da linha de comando.
 */
void usage(int argc, char **argv) {
    printf("usage: %s <ipv4|ipv6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Estrutura para representar uma ação do jogador
struct action {
    int type;
    int moves[100];
    int board[10][10];
};

// Estrutura para representar uma posição no tabuleiro
struct Position {
    int x;
    int y;
};

// Estrutura para um nó da fila usada no BFS
struct QueueNode {
    struct Position pos;
    struct QueueNode *next;
    int path[MAX_PATH];
    int path_length;
};

/**
 * @brief Cria um novo nó para a fila do BFS.
 *
 * @param x Coordenada x da posição.
 * @param y Coordenada y da posição.
 * @return Ponteiro para o novo nó criado.
 */
struct QueueNode *createNode(int x, int y) {
    struct QueueNode *node =
        (struct QueueNode *)malloc(sizeof(struct QueueNode));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->pos.x = x;
    node->pos.y = y;
    node->next = NULL;
    node->path_length = 0;
    return node;
}

/**
 * @brief Encontra o caminho mais curto até a saída usando BFS.
 *
 * @param start_x Coordenada x da posição inicial.
 * @param start_y Coordenada y da posição inicial.
 * @param hint String onde o caminho encontrado será armazenado.
 * @return Ponteiro para a string hint.
 */
char *find_path_to_exit(int start_x, int start_y, char *hint) {
    // Aloca a matriz visited dinamicamente
    int(*visited)[board_size] = calloc(board_size, sizeof(*visited));
    if (!visited) {
        strcpy(hint, "Error: Memory allocation failed!");
        return hint;
    }

    struct QueueNode *queue = createNode(start_x, start_y);
    struct QueueNode *front = queue, *rear = queue;

    // Direções possíveis: cima, direita, baixo, esquerda (em sentido horário)
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    char *directions[] = {"up", "right", "down", "left"};

    visited[start_y][start_x] = 1;

    while (front != NULL) {
        struct Position current = front->pos;
        int path_len = front->path_length;
        int current_path[MAX_PATH];
        memcpy(current_path, front->path, sizeof(int) * path_len);

        // Se encontrou a saída
        if (game_board[current.y][current.x] == EXIT) {
            strcpy(hint, "Hint: ");
            for (int i = 0; i < path_len; i++) {
                if (i > 0)
                    strcat(hint, ", ");
                strcat(hint, directions[current_path[i]]);
            }

            // Limpa a fila
            while (front != NULL) {
                struct QueueNode *temp = front;
                front = front->next;
                free(temp);
            }

            // Libera a memória alocada
            free(visited);
            return hint;
        }

        // Tenta todas as direções possíveis
        for (int i = 0; i < 4; i++) {
            int new_x = current.x + dx[i];
            int new_y = current.y + dy[i];

            if (new_x >= 0 && new_x < board_size && new_y >= 0 &&
                new_y < board_size && !visited[new_y][new_x] &&
                (game_board[new_y][new_x] == PATH ||
                 game_board[new_y][new_x] == EXIT)) {

                visited[new_y][new_x] = 1;
                struct QueueNode *newNode = createNode(new_x, new_y);

                // Copia o caminho anterior e adiciona a nova direção
                memcpy(newNode->path, current_path, sizeof(int) * path_len);
                newNode->path[path_len] = i;
                newNode->path_length = path_len + 1;

                rear->next = newNode;
                rear = newNode;
            }
        }

        struct QueueNode *temp = front;
        front = front->next;
        free(temp);
    }

    // Libera a memória alocada
    free(visited);
    strcpy(hint, "No path to exit found!");
    return hint;
}

/**
 * @brief Lê o mapa do labirinto a partir do arquivo.
 *
 * Esta função lê o mapa do arquivo especificado em MAP_FILE, determinando
 * automaticamente o tamanho do tabuleiro e verificando se o formato é válido.
 * O mapa é armazenado na variável global game_board.
 *
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int read_map_from_file() {
    FILE *file = fopen(MAP_FILE, "r");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    char line[256];
    int rows = 0;
    int cols = 0;

    // Lê a primeira linha para determinar o número de colunas
    if (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, " \t\n");
        while (token != NULL) {
            cols++;
            token = strtok(NULL, " \t\n");
        }
        rows = 1;
    }

    // Conta o número de linhas
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) > 1) { // Ignora linhas vazias
            rows++;
        }
    }

    // Verifica se o tabuleiro é quadrado e tem tamanho válido
    if (rows != cols || rows < MIN_BOARD_SIZE || rows > MAX_BOARD_SIZE) {
        fprintf(stderr,
                "Error: Invalid map format in %s. Board must be square between "
                "[%d x %d] and [%d x %d].\n",
                MAP_FILE, MIN_BOARD_SIZE, MIN_BOARD_SIZE, MAX_BOARD_SIZE,
                MAX_BOARD_SIZE);
        fclose(file);
        return -1;
    }

    board_size = rows;

    // Volta ao início do arquivo para ler o tabuleiro
    rewind(file);

    int entrance_found = 0;
    int exit_found = 0;

    // Lê o mapa do arquivo
    for (int i = 0; i < board_size; i++) {
        for (int j = 0; j < board_size; j++) {
            int value;
            if (fscanf(file, "%d", &value) != 1) {
                fprintf(stderr, "Error: Invalid map format in %s.\n", MAP_FILE);
                fclose(file);
                return -1;
            }

            // Verifica se o valor é válido
            if (value < 0 || value > 5) {
                fprintf(stderr,
                        "Error: Invalid cell value '%d' in map file %s.\n",
                        value, MAP_FILE);
                fclose(file);
                return -1;
            }

            game_board[i][j] = value;

            // Conta entradas e saídas
            if (value == ENTRANCE) {
                if (entrance_found > 0) {
                    fprintf(stderr,
                            "Error: Multiple entrances found in map file %s.\n",
                            MAP_FILE);
                    fclose(file);
                    return -1;
                }
                entrance_found++;
                player_x = j;
                player_y = i;
            } else if (value == EXIT) {
                if (exit_found > 0) {
                    fprintf(stderr,
                            "Error: Multiple exits found in map file %s.\n",
                            MAP_FILE);
                    fclose(file);
                    return -1;
                }
                exit_found++;
            }
        }
    }

    // Verifica se há exatamente uma entrada e uma saída
    if (entrance_found != 1 || exit_found != 1) {
        fprintf(stderr,
                "Error: Map must have exactly one entrance and one exit.\n");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

/**
 * @brief Inicializa o tabuleiro do jogo.
 *
 * Esta função inicializa o tabuleiro lendo o mapa do arquivo e configurando
 * as células descobertas inicialmente ao redor da posição inicial do jogador.
 *
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int init_board() {
    memset(game_board, 0, sizeof(game_board));
    memset(discovered, 0, sizeof(discovered));

    if (read_map_from_file() != 0) {
        fprintf(stderr, "Failed to initialize game board\n");
        return -1;
    }

    // Marca a posição inicial e células adjacentes como descobertas
    discovered[player_y][player_x] = 1;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int ny = player_y + dy;
            int nx = player_x + dx;
            if (ny >= 0 && ny < board_size && nx >= 0 && nx < board_size) {
                discovered[ny][nx] = 1;
            }
        }
    }

    game_started = 1;
    return 0;
}

/**
 * @brief Obtém o caractere de representação para uma célula do tabuleiro.
 *
 * @param cell_type Tipo da célula (WALL, PATH, etc.).
 * @param x Coordenada x da célula.
 * @param y Coordenada y da célula.
 * @return Caractere que representa a célula.
 */
char get_cell_char(int cell_type, int x, int y) {
    if (!show_full_map && !discovered[y][x]) {
        return '?';
    }

    if (x == player_x && y == player_y) {
        if (game_board[y][x] == EXIT) {
            return 'X';
        }
        return '+';
    }

    switch (cell_type) {
    case WALL:
        return '#';
    case PATH:
        return '_';
    case ENTRANCE:
        return '>';
    case EXIT:
        return 'X';
    default:
        return ' ';
    }
}

/**
 * @brief Gera uma string representando o estado atual do tabuleiro.
 *
 * @param map_str Buffer onde a string será armazenada.
 */
void get_map_string(char *map_str) {
    strcpy(map_str, "");
    for (int i = 0; i < board_size; i++) {
        for (int j = 0; j < board_size; j++) {
            char cell = get_cell_char(game_board[i][j], j, i);
            char temp[3] = {cell, '\t', '\0'};
            strcat(map_str, temp);
        }
        strcat(map_str, "\n");
    }
}

/**
 * @brief Obtém os movimentos possíveis a partir da posição atual.
 *
 * @param x Coordenada x atual.
 * @param y Coordenada y atual.
 * @param moves Buffer onde a string de movimentos será armazenada.
 */
void get_possible_moves(int x, int y, char *moves) {
    strcpy(moves, "possible moves: ");
    int first_move = 1;

    // Verifica movimentos possíveis em ordem horária começando por cima
    if (y - 1 >= 0 &&
        (game_board[y - 1][x] == PATH || game_board[y - 1][x] == EXIT)) {
        strcat(moves, "up");
        first_move = 0;
    }
    if (x + 1 < board_size &&
        (game_board[y][x + 1] == PATH || game_board[y][x + 1] == EXIT)) {
        if (!first_move)
            strcat(moves, ", ");
        strcat(moves, "right");
        first_move = 0;
    }
    if (y + 1 < board_size &&
        (game_board[y + 1][x] == PATH || game_board[y + 1][x] == EXIT)) {
        if (!first_move)
            strcat(moves, ", ");
        strcat(moves, "down");
        first_move = 0;
    }
    if (x - 1 >= 0 &&
        (game_board[y][x - 1] == PATH || game_board[y][x - 1] == EXIT)) {
        if (!first_move)
            strcat(moves, ", ");
        strcat(moves, "left");
    }
}

/**
 * @brief Processa um comando recebido do cliente.
 *
 * Esta função interpreta e executa os comandos recebidos do cliente,
 * atualizando o estado do jogo e gerando a resposta apropriada.
 *
 * @param cmd Comando recebido do cliente.
 * @param response Buffer onde a resposta será armazenada.
 */
void process_command(char *cmd, char *response) {
    if (strcmp(cmd, "start") == 0) {
        printf("starting new game\n");
        if (init_board() != 0) { // Verifica se a inicialização foi bem-sucedida
            strcpy(response, ""); // Não envia resposta em caso de falha
            return;
        }
        game_completed = 0;
        char moves[BUFSZ];
        get_possible_moves(player_x, player_y, moves);
        strcat(response, moves);
    } else if (!game_started) {
        strcpy(response, "error: start the game first!");
        return;
    } else if (strcmp(cmd, "right") == 0) {
        if (player_x + 1 < board_size &&
            (game_board[player_y][player_x + 1] == PATH ||
             game_board[player_y][player_x + 1] == EXIT)) {
            player_x++;
        } else {
            strcpy(response, "error: you cannot go this way\n");
        }
        char moves[BUFSZ];
        get_possible_moves(player_x, player_y, moves);
        strcat(response, moves);
    } else if (strcmp(cmd, "left") == 0) {
        if (player_x - 1 >= 0 && (game_board[player_y][player_x - 1] == PATH ||
                                  game_board[player_y][player_x - 1] == EXIT)) {
            player_x--;
        } else {
            strcpy(response, "error: you cannot go this way\n");
        }
        char moves[BUFSZ];
        get_possible_moves(player_x, player_y, moves);
        strcat(response, moves);
    } else if (strcmp(cmd, "up") == 0) {
        if (player_y - 1 >= 0 && (game_board[player_y - 1][player_x] == PATH ||
                                  game_board[player_y - 1][player_x] == EXIT)) {
            player_y--;
        } else {
            strcpy(response, "error: you cannot go this way\n");
        }
        char moves[BUFSZ];
        get_possible_moves(player_x, player_y, moves);
        strcat(response, moves);
    } else if (strcmp(cmd, "down") == 0) {
        if (player_y + 1 < board_size &&
            (game_board[player_y + 1][player_x] == PATH ||
             game_board[player_y + 1][player_x] == EXIT)) {
            player_y++;
        } else {
            strcpy(response, "error: you cannot go this way\n");
        }
        char moves[BUFSZ];
        get_possible_moves(player_x, player_y, moves);
        strcat(response, moves);
    } else if (strcmp(cmd, "map") == 0) {
        get_map_string(response);
    } else if (strcmp(cmd, "hint") == 0) {
        char hint[BUFSZ];
        find_path_to_exit(player_x, player_y, hint);
        strcpy(response, hint);
    } else if (strcmp(cmd, "reset") == 0) {
        init_board();
        game_completed = 0;
        strcpy(response, "");
        char moves[BUFSZ];
        get_possible_moves(player_x, player_y, moves);
        strcat(response, moves);
        printf("starting new game\n"); // Adiciona esta linha
    } else if (strcmp(cmd, "exit") == 0) {
        game_started = 0;
        game_completed = 0;
        strcpy(response, "");
        printf("client disconnected\n");
        return;
    } else {
        strcpy(response, "error: command not found");
    }

    // Atualiza células descobertas após movimentos
    if (strcmp(cmd, "right") == 0 || strcmp(cmd, "left") == 0 ||
        strcmp(cmd, "up") == 0 || strcmp(cmd, "down") == 0) {
        // Descobre células adjacentes à nova posição do jogador
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int ny = player_y + dy;
                int nx = player_x + dx;
                if (ny >= 0 && ny < board_size && nx >= 0 && nx < board_size) {
                    discovered[ny][nx] = 1;
                }
            }
        }
    }

    // Verifica se o jogador chegou à saída
    if (game_board[player_y][player_x] == EXIT) {
        game_completed = 1;
        show_full_map = 1;
        strcat(response, "\nYou escaped!\n");
        char map[BUFSZ];
        get_map_string(map);
        strcat(response, map);
        show_full_map = 0;
    }
}

/**
 * @brief Função principal do servidor.
 *
 * Inicializa o servidor, aceita conexões de clientes e gerencia
 * a comunicação com eles.
 */
int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    //  Inicializa a estrutura de endereço do servidor
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    // Cria o socket para realizar a comunicação
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    // Define a opção de reutilização do endereço
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    // Associa o socket ao endereço passado
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    // Coloca o socket em modo de escuta
    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        // Aceita a conexão do cliente
        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);

        printf("client connected\n");

        while (1) {
            char buf[BUFSZ];
            char response[BUFSZ];
            memset(buf, 0, BUFSZ);
            memset(response, 0, BUFSZ);

            // Recebe o comando do cliente
            size_t count = recv(csock, buf, BUFSZ - 1, 0);
            if (count <= 0) {
                break; // Cliente desconectou
            }

            // Processa o comando
            process_command(buf, response);

            // Envia a resposta
            count = send(csock, response, strlen(response) + 1, 0);
            if (count != strlen(response) + 1) {
                logexit("send");
            }

            if (strcmp(buf, "exit") == 0) {
                break;
            }
        }

        close(csock);
    }

    exit(EXIT_SUCCESS);
}