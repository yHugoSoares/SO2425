# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -I$(INCLUDE_DIR) $(shell pkg-config --cflags glib-2.0)
LDFLAGS = $(shell pkg-config --libs glib-2.0)

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = includes
TMP_DIR = tmp
DATASET_DIR = Gdataset  # Usando apenas o diretório Gdataset que existe

# Source files
SRC_FILES = \
    $(SRC_DIR)/dclient.c \
    $(SRC_DIR)/dserver.c \
    $(SRC_DIR)/handle_request.c \
    $(SRC_DIR)/cache.c \
    $(SRC_DIR)/index.c \
    $(SRC_DIR)/common.c

# Object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Executables
CLIENT_EXEC = $(BIN_DIR)/dclient
SERVER_EXEC = $(BIN_DIR)/dserver

# Build rules
all: prepare $(CLIENT_EXEC) $(SERVER_EXEC)

prepare:
	@echo "[PREPARE] Creating build and bin directories."
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(TMP_DIR)
	@echo "[PREPARE] Checking if dataset directory exists."
	@if [ ! -d "$(DATASET_DIR)" ]; then \
		echo "[WARNING] Dataset directory $(DATASET_DIR) does not exist!"; \
		echo "[WARNING] Trying alternative paths..."; \
		if [ -d "Gdataset" ]; then \
			echo "[INFO] Using Gdataset directory."; \
			DATASET_DIR="Gdataset"; \
		else \
			echo "[ERROR] Could not find a valid dataset directory!"; \
			exit 1; \
		fi; \
	fi

$(CLIENT_EXEC): $(BUILD_DIR)/dclient.o $(BUILD_DIR)/common.o
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $(CLIENT_EXEC) $(LDFLAGS)

$(SERVER_EXEC): $(BUILD_DIR)/dserver.o $(BUILD_DIR)/handle_request.o $(BUILD_DIR)/cache.o $(BUILD_DIR)/common.o $(BUILD_DIR)/index.o
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $(SERVER_EXEC) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

server: prepare $(SERVER_EXEC)
	@echo "[SERVER] Starting server with dataset directory $(DATASET_DIR) and cache size 1024"
	./$(BIN_DIR)/dserver $(DATASET_DIR) 1024

# Comando para listar arquivos no diretório do dataset
list-files:
	@echo "[INFO] Listing files in dataset directory:"
	@find $(DATASET_DIR) -type f | sort

clean:
	@echo "[CLEAN] Cleaning build directory."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

cleanall: clean
	@echo "[CLEAN] Removing temporary files."
	rm -rf $(TMP_DIR)
	@echo "[CLEAN] Removing metadata file."
	rm -f metadata.dat

.PHONY: all clean cleanall prepare server list-files
