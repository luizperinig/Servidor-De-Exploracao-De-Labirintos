/**
 * @file common.h
 * @brief Arquivo de cabeçalho contendo definições e protótipos de funções comuns 
 * para o cliente e o servidor do jogo de labirinto.
 *
 * Este arquivo define estruturas de dados e protótipos de funções utilitárias
 * compartilhadas entre o cliente e o servidor, como manipulação de endereços
 * de rede e tratamento de erros.
 */
#pragma once

#include <stdlib.h>
#include <arpa/inet.h>

/**
 * @brief Função para registrar um erro e encerrar o programa.
 * 
 * @param msg Mensagem de erro a ser exibida.
 */
void logexit(const char *msg);

/**
 * @brief Função para analisar uma string de endereço e porta.
 *
 * @param addrstr String contendo o endereço IP.
 * @param portstr String contendo a porta.
 * @param storage Ponteiro para a estrutura sockaddr_storage onde o endereço
 *                analisado será armazenado.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

/**
 * @brief Função para converter um endereço de socket para string.
 *
 * @param addr Ponteiro para a estrutura sockaddr contendo o endereço.
 * @param str Ponteiro para o buffer onde a string será armazenada.
 * @param strsize Tamanho do buffer.
 */
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

/**
 * @brief Função para inicializar a estrutura de endereço do servidor.
 *
 * @param proto String indicando o protocolo ("v4" ou "v6").
 * @param portstr String contendo a porta.
 * @param storage Ponteiro para a estrutura sockaddr_storage onde o endereço
 *                será armazenado.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);