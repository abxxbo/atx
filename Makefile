CC := cc
CFLAGS := -Wall -Wextra -pedantic -std=c99

## Default to usr
## Change with
## sudo make install INSTALL_PREFIX=[directory]
INSTALL_PREFIX ?= /usr

.PHONY: atx

atx: atx.c
	@echo CC $^
	@$(CC) $^ -o $@ $(CFLAGS)

clean: ./atx
	@echo RM $^
	@rm $^

install: ./atx
	ifeq($(INSTALL_PREFIX),"/usr")
		@echo CP $^ => /usr/local/bin/atx
		@sudo cp ./atx /usr/local/bin/atx
	else
		@echo CP $^ => $(INSTALL_PREFIX)/atx
		@sudo cp ./atx $(INSTALL_PREFIX)/atx
	endif
