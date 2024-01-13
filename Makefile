CC = cc 
CFLAGS = -Wall -Wextra -pedantic
LIBS = -lcurses
SRC = src/
OUT = build/

build: $(SRC)main.c
	@mkdir -p $(OUT) 
	$(CC) $^ $(LIBS) -o $(OUT)main $(CFLAGS)

debug: $(SRC)main.c
	@mkdir -p $(OUT) 
	$(CC) $^ $(LIBS) -o $(OUT)debug $(CFLAGS) -ggdb2 
