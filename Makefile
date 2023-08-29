CC     = gcc
CDFLAGS = -g -Wall -Werror -pedantic-errors -Iinclude
SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build
CFLAGS = -Wall -Werror -pedantic-errors -Iinclude -O3

.PHONY: all clean debug
all: build
build: $(SRC_DIR)/minishell.c
	mkdir -p $(BUILD_DIR)/release
	$(CC) $(CFLAGS) $(SRC_DIR)/minishell.c -o $(BUILD_DIR)/release/minishell
clean:
	rm -rf $(BUILD_DIR)
debug: $(SRC_DIR)/minishell.c
	mkdir -p $(BUILD_DIR)/debug
	$(CC) $(CDFLAGS) $(SRC_DIR)/minishell.c -o $(BUILD_DIR)/debug/minishell