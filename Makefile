CC = gcc
LD = gcc
CFLAGS = -std=c23 -Iinc
LDFLAGS =
SRC = $(shell find src/ -name '*.c')
OBJ = $(patsubst src/%, build/%, $(SRC:.c=.o))
TARGET = main

RESET = \033[0m
GREEN = \033[32m
YELLOW = \033[33m
RED = \033[31m

all: $(TARGET)
	@echo "${GREEN}Building project...${RESET}"
	@$(LD) $(OBJ) -o $@ $(LDFLAGS)
	@echo "${GREEN}Build completed successfully.${RESET}"

$(TARGET): $(OBJ)
	@echo "${YELLOW}Linking...${RESET}"
	@$(LD) $(OBJ) -o $@ $(LDFLAGS)
	@echo "${GREEN}Linking done.${RESET}"

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "${YELLOW}Compiling $<...${RESET}"
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "${GREEN}Compiled $<.${RESET}"

clean:
	@echo "${RED}Cleaning up...${RESET}"
	@clear
	@rm -rf build $(TARGET)
	@echo "${RED}Clean up done.${RESET}"

