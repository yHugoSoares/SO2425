# ========== TOOLCHAIN ==========
CC      = gcc
CFLAGS  = -Wall -Werror -Iincludes
LDFLAGS = 

# ========== PROJECT STRUCTURE ==========
SRC_DIR   = src
BUILD_DIR = build
EXEC_DIR  = executables
RES_DIR   = resultados

# ========== AUTOMATED FILE LISTS ==========
SRCS      = $(wildcard $(SRC_DIR)/*.c)
CLIENT_OBJ= $(BUILD_DIR)/client.o 
SERVER_OBJ= $(BUILD_DIR)/server.o
COMMON_OBJ= $(BUILD_DIR)/common.o

# ========== BUILD TARGETS ==========
.PHONY: all clean

all: $(EXEC_DIR)/client $(EXEC_DIR)/server

$(EXEC_DIR)/client: $(CLIENT_OBJ) $(COMMON_OBJ)
	@mkdir -p $(@D)
	@echo "\033[0;34m[CLIENT]\033[0m 🔗 Linking $@"
	@$(CC) $^ -o $@ $(LDFLAGS)

$(EXEC_DIR)/server: $(SERVER_OBJ) $(COMMON_OBJ)
	@mkdir -p $(@D)
	@echo "\033[0;35m[SERVER]\033[0m 🔗 Linking $@"
	@$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "\033[0;36m[COMPILE]\033[0m 📦 $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ========== CLEANER ==========
clean:
	@echo "\033[0;33m[CLEAN]\033[0m 🧹 Removing artifacts..."
	@rm -rfv $(BUILD_DIR) $(EXEC_DIR) $(RES_DIR)/* | sed 's/^/  /'
	@echo "\033[0;32m[SUCCESS]\033[0m ✅ Project spotless!\n"

.DEFAULT_GOAL = all