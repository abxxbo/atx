## bash
SHELL := bash
.ONE_SHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

CC := cc
CFLAGS := -Wall -Wextra -pedantic -std=c99 -Iinclude/editor/

## Default to usr
## Change with
## sudo make install INSTALL_PREFIX=[directory]
INSTALL_PREFIX ?= /usr

.PHONY: atx

atx: src/atx.c
	@echo CC $^
	@$(CC) $^ -o ./$@ $(CFLAGS)

clean:
	@echo RM atx
	@rm atx

install:
	@echo CP ./atx => /usr/local/bin/atx
	@sudo cp ./atx /usr/local/bin/atx
