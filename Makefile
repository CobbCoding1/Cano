CC = cc 
CFLAGS = -Wall -Wextra -pedantic
LIBS = -lcurses
SRC = src/
OUT = build/

build: $(SRC)main.c
	$(CC) $^ $(LIBS) -o $(OUT)main $(CFLAGS)

debug: $(SRC)main.c
	$(CC) $^ $(LIBS) -o $(OUT)main $(CFLAGS) -ggdb2 
