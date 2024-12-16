CC = gcc
AS = nasm
LD = gcc

CFLAGS = -std=c23 -Iinc -Wall -Wextra -Werror -pedantic
ASFLAGS = -f elf64
LDFLAGS =

SRC_DIR = src
BUILD_DIR = build

C_SRC = $(shell find $(SRC_DIR) -name '*.c')
ASM_SRC = $(shell find $(SRC_DIR) -name '*.asm')
SRC = $(C_SRC) $(ASM_SRC)
OBJ = $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(C_SRC:.c=.o) $(ASM_SRC:.asm=.o))

TARGET = entc

RESET = \033[0m
GREEN = \033[32m
YELLOW = \033[33m
RED = \033[31m

.PHONY: all clean reset

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "${YELLOW}Linking target: $@...${RESET}"
	@$(LD) $(OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "${YELLOW}Compiling: $<...${RESET}"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "${YELLOW}Assembling: $<...${RESET}"
	@$(AS) $(ASFLAGS) $< -o $@

clean:
	@clear
	@echo "${RED}Cleaning up...${RESET}"
	@rm -rf $(BUILD_DIR) $(TARGET)

reset: clean all