CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Wpedantic
DBFLAGS = -ggdb2
LDFLAGS = -lncurses -lm
SRC = src/
OUT = build/

cano: $(SRC)main.c 
	@mkdir -p $(OUT) 
	$(CC) -o $(OUT)$@ $(CFLAGS) $^ $(LDFLAGS)

debug: $(SRC)main.c
	@mkdir -p $(OUT) 
	$(CC) -o $(OUT)$@ $(CFLAGS) $(DBFLAGS) $^ $(LDFLAGS)
