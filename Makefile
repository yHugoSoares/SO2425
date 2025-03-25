# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror

# Directories
SRC_DIR = src
BUILD_DIR = build
EXEC_DIR = executables
INCLUDE_DIR = include
RESULT_DIR = resultados

# Source files
SRC_FILES = \
	$(SRC_DIR)/client.c \
	$(SRC_DIR)/server.c \
	$(SRC_DIR)/cache_management.c \
	$(SRC_DIR)/document_operations.c \
	$(SRC_DIR)/search_operations.c \


# Object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Executables
CLIENT_EXEC = $(EXEC_DIR)/client
SERVER_EXEC = $(EXEC_DIR)/server

# Build rules
all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(BUILD_DIR)/client.o
	@mkdir -p $(EXEC_DIR)
	$(CC) $(BUILD_DIR)/client.o -o $(CLIENT_EXEC) $(LDFLAGS)

$(SERVER_EXEC): $(BUILD_DIR)/server.o
	@mkdir -p $(EXEC_DIR)
	$(CC) $(BUILD_DIR)/server.o -o $(SERVER_EXEC) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "[CLEAN] Cleaning build directory."
	rm -rf $(BUILD_DIR) $(EXEC_DIR)
	@echo "[CLEAN] Cleaning resultados directory."
	rm -rf $(RESULT_DIR)/*

.PHONY: all clean
