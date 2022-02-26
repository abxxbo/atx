CC := cc
CFLAGS := -Wall -Wextra -pedantic -std=c99

atx: atx.c
	$(CC) $^ -o $@ $(CFLAGS)
