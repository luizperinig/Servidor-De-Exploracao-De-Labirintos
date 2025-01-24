# Labirinto - Trabalho Prático 1 de Sistemas Computacionais

Este projeto implementa um jogo de labirinto com comunicação cliente-servidor via sockets em C, utilizando a interface POSIX. O jogo permite a exploração gradual do labirinto, fornecendo dicas de caminho e gerenciando o estado do jogo entre cliente e servidor.

## Compilação e Execução

O projeto foi desenvolvido em ambiente Linux (WSL - Ubuntu).  Compile o código usando o Makefile com o seguinte comando no terminal:

```bash
make
```
Para executar o servidor e o cliente, utilize os seguintes comandos (lembre-se do prefixo /bin/):

**IPv4:**
```bash
# Terminal 1 (Servidor):
/bin/server v4 51511 -i input/in.txt

# Terminal 2 (Cliente):
/bin/client 127.0.0.1 51511
```
**IPv6:**
```bash
# Terminal 1 (Servidor):
/bin/server v6 51511 -i input/in.txt

# Terminal 2 (Cliente):
/bin/client ::1 51511
```
Onde:

* **v4 ou v6**: Especifica a versão do protocolo IP. </br>

* **51511**: Número da porta (pode ser alterado).</br>

* **-i input/in.txt**: Caminho para o arquivo de texto que define o labirinto.</br>

</br>

## Arquivos do Projeto</br>

* **server.c**: Implementação do servidor.</br>

* **client.c**: Implementação do cliente.</br>

* **common.c**: Funções auxiliares compartilhadas entre cliente e servidor.</br>

* **common.h**: Arquivo de cabeçalho para common.c.</br>

* **input/in.txt**: Arquivo de exemplo para o labirinto.</br>

</br>

## Funcionalidades</br>

* **Exploração gradual do labirinto**: Células não visitadas são ocultadas, revelando-se à medida que o jogador explora.</br>

* **Sistema de dicas**: Fornece o caminho até a saída usando o algoritmo BFS (Breadth-First Search).</br>

* **Gerenciamento de estado do jogo**: Mantém a consistência entre cliente e servidor, mesmo após vitória ou reinício.</br>

* **Tratamento de erros**: Robustez contra entradas inválidas e condições inesperadas.</br>

* **Detecção automática do tamanho do tabuleiro**: Suporta tabuleiros de 5x5 até 10x10.</br>

## Desafios e Soluções</br>

O projeto abordou diversos desafios, incluindo:</br>

* **Gerenciamento de conexões**: Manutenção de uma conexão estável usando sockets TCP e tratamento adequado de desconexões.</br>

* **Detecção automática do tamanho do tabuleiro**: Implementação de um algoritmo para determinar o tamanho do tabuleiro a partir do arquivo de entrada e validação do formato.</br>

* **Implementação do sistema de descoberta**: Criação de uma matriz para controlar as células descobertas e lógica para revelar células adjacentes ao jogador, simulando a "névoa de guerra".</br>

* **Algoritmo de busca para dicas**: Implementação do algoritmo BFS para encontrar o menor caminho até a saída e tradução do caminho em direções de movimento.</br>

* **Gerenciamento de estado do jogo**: Implementação de variáveis de controle e tratamento especial para comandos pós-vitória, garantindo a sincronização entre cliente e servidor.</br>

* **Tratamento de erros**: Validação de entradas, tratamento de erros de alocação de memória e mensagens de erro informativas para auxiliar o usuário.</br>


## Referências</br>

* Youtube: Playlist "Introdução à Programação em Redes" - Professor Ítalo Cunha</br>

* Livros: "TCP/IP Sockets in C, Second Edition Practical Guide for Programmers" - Michael J. Donahoo and Kenneth L. Calvert</br>

* Web:</br>
BFS Algorithm</br>
Queue Implementation</br>
Doxygen Manual
