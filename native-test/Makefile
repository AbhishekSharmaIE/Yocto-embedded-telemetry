SRC = ../meta-mysoftware/recipes-myapps/bootinfo-server/files/bootinfo-server.c
CC ?= gcc
CFLAGS += -Wall -Wextra

bootinfo-server: $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

run: bootinfo-server
	./bootinfo-server

clean:
	rm -f bootinfo-server

.PHONY: run clean
