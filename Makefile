## Default goal is just building the editor
.DEFAULT_GOAL := editor


## Directories / other variables
BINARY_NAME := atx
SRC_DIR 		:= src/

## Main ATX editor variables and targets
CC 			:= cc
CFLAGS	:= -Iinclude/ -Iinclude/editor/ -Wall -Wextra \
					 -pedantic -std=c2x -g -O2

editor: $(SRC_DIR)/editor/atx.c
	@echo CC $^
	@$(CC) $^ $(CFLAGS) -o ./$(BINARY_NAME)

## Other targets
clean: $(BINARY_NAME)
	@rm $^

install: $(BINARY_NAME)
	@sudo cp $^ /usr/local/bin/$^
