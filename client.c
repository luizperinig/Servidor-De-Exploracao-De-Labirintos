/**
 * @file client.c
 * @brief Implementação do cliente do jogo de labirinto.
 *
 * Este arquivo contém a implementação do cliente do jogo de labirinto,
 * responsável por estabelecer conexão com o servidor, enviar comandos
 * e receber/exibir as respostas. O cliente gerencia a interface com o
 * usuário e mantém o estado do jogo localmente.
 */

#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Flag global para controlar o estado de vitória do jogo
int game_won = 0;

// Tamanho máximo do buffer de mensagens
#define BUFSZ 1024

/**
 * @brief Exibe a mensagem de uso do programa e encerra a execução.
 *
 * @param argc Número de argumentos da linha de comando.
 * @param argv Vetor de strings contendo os argumentos da linha de comando.
 */
void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

/**
 * @brief Estrutura para representar uma ação do jogador.
 * 
 * Esta estrutura é usada para manter informações sobre as ações
 * do jogador e o estado do tabuleiro.
 */
struct action {
    int type;              // Tipo da ação
    int moves[100];        // Array de movimentos
    int board[10][10];     // Estado do tabuleiro
};

/**
 * @brief Função principal do cliente.
 *
 * Estabelece conexão com o servidor, processa comandos do usuário
 * e gerencia o estado do jogo localmente.
 *
 * @param argc Número de argumentos da linha de comando.
 * @param argv Vetor de strings contendo os argumentos da linha de comando.
 * @return 0 em caso de sucesso, outro valor em caso de erro.
 */
int main(int argc, char **argv) {
    // Verifica os argumentos da linha de comando
    if (argc != 3) {
        usage(argc, argv);
    }

    // Inicializa a estrutura de endereço do servidor
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    // Flag para controlar se o jogo está ativo
    int game_active = 0;

    // Cria o socket para comunicação com o servidor
    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    // Conecta ao servidor
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    // Loop principal do cliente
    while (1) {
        // Lê comando do usuário
        char cmd[BUFSZ];
        fgets(cmd, BUFSZ - 1, stdin);
        cmd[strcspn(cmd, "\n")] = 0;  // Remove o caractere de nova linha

        // Se o jogo foi vencido, aceita apenas 'reset' ou 'exit'
        if (game_won && strcmp(cmd, "reset") != 0 && strcmp(cmd, "exit") != 0) {
            continue;
        }

        // Verifica se o comando é válido
        if (strcmp(cmd, "start") == 0 || strcmp(cmd, "right") == 0 ||
            strcmp(cmd, "left") == 0 || strcmp(cmd, "up") == 0 ||
            strcmp(cmd, "down") == 0 || strcmp(cmd, "map") == 0 ||
            strcmp(cmd, "hint") == 0 || strcmp(cmd, "reset") == 0 ||
            strcmp(cmd, "exit") == 0) {

            // Verifica se o jogo foi iniciado
            if (!game_active && strcmp(cmd, "start") != 0) {
                printf("error: start the game first\n");
                continue;
            }

            // Envia o comando para o servidor
            size_t count = send(s, cmd, strlen(cmd) + 1, 0);
            if (count != strlen(cmd) + 1) {
                logexit("send");
            }

            // Recebe a resposta do servidor
            char buf[BUFSZ];
            memset(buf, 0, BUFSZ);
            count = recv(s, buf, BUFSZ - 1, 0);

            if (count > 0) {
                // Exibe a resposta do servidor
                printf("\n%s\n", buf);

                // Atualiza o estado do jogo com base no comando
                if (strcmp(cmd, "start") == 0) {
                    game_active = 1;
                    game_won = 0;
                } else if (strcmp(cmd, "exit") == 0) {
                    close(s);
                    exit(EXIT_SUCCESS);
                } else if (strcmp(cmd, "reset") == 0) {
                    game_won = 0;    // Reseta o estado de vitória
                    game_active = 1; // Reativa o jogo
                } else if (strstr(buf, "You escaped!") != NULL) {
                    game_won = 1;    // Marca o jogo como vencido
                }
            }
        } else {
            // Comando inválido
            printf("error: command not found\n");
        }
    }

    // Fecha o socket (este ponto só é alcançado em caso de erro)
    close(s);
    return 0;
}