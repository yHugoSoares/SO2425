# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -I$(INCLUDE_DIR)

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = includes
RESULT_DIR = resultados

# Source files
SRC_FILES = \
	$(SRC_DIR)/dclient.c \
	$(SRC_DIR)/dserver.c \
	$(SRC_DIR)/handle_request.c \
	$(SRC_DIR)/cache.c

# Object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Executables
CLIENT_EXEC = $(BIN_DIR)/dclient
SERVER_EXEC = $(BIN_DIR)/dserver

# Build rules
all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(BUILD_DIR)/dclient.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(BUILD_DIR)/dclient.o -o $(CLIENT_EXEC) $(LDFLAGS)

$(SERVER_EXEC): $(BUILD_DIR)/dserver.o $(BUILD_DIR)/handle_request.o $(BUILD_DIR)/cache.o $(BUILD_DIR)/common.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(BUILD_DIR)/dserver.o $(BUILD_DIR)/handle_request.o $(BUILD_DIR)/cache.o $(BUILD_DIR)/common.o -o $(SERVER_EXEC) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_EXEC)
	./$(BIN_DIR)/dserver metadata 1024

clean:
	@echo "[CLEAN] Cleaning build directory."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "[CLEAN] Cleaning resultados directory."
	rm -rf $(RESULT_DIR)/*

.PHONY: all clean