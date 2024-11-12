# Compilador e flags
CC = gcc
CFLAGS = -Wall

# Diretórios
BIN_DIR = bin

# Arquivos fonte
SERVER_SRC = server.c common.c
CLIENT_SRC = client.c common.c

# Arquivos objeto
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Binários
SERVER = $(BIN_DIR)/server
CLIENT = $(BIN_DIR)/client

# Regra padrão
all: directories $(SERVER) $(CLIENT)

# Cria o diretório bin se não existir
directories:
	mkdir -p $(BIN_DIR)

# Compila o servidor
$(SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Compila o cliente
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Regra para arquivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpa os arquivos compilados
clean:
	rm -f *.o
	rm -rf $(BIN_DIR)

# Define os alvos que não são arquivos
.PHONY: all clean directories