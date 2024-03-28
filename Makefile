BUILD_DIR = build

CC = gcc
CFLAGS = -Wall -Wextra
CFLAGS += -pedantic -Wpedantic

LDLIBS = -lncurses -lm

VPATH += src
SRC += main.c

OBJ := $(SRC:%.c=$(BUILD_DIR)/release-objs/%.o)
OBJ_DEBUG :=$(SRC:%.c=$(BUILD_DIR)/debug-objs/%.o)

all: cano
cano: $(BUILD_DIR)/cano
debug: $(BUILD_DIR)/debug

.PHONY: all cano debug

$(BUILD_DIR)/release-objs/%.o: %.c
	@ mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $< || exit 1

$(BUILD_DIR)/debug-objs/%.o: %.c
	@ mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $< || exit 1

$(BUILD_DIR)/cano: $(OBJ)
	@ mkdir -p $(dir $@)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/debug: CFLAGS += -ggdb2
$(BUILD_DIR)/debug: $(OBJ_DEBUG)
	@ mkdir -p $(dir $@)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(OBJS) $(OBJ_DEBUG)

fclean:
	$(RM) -r $(BUILD_DIR)

re: fclean
	$(MAKE) all

install: all
	@ install -v -D -t ~/.local/share/cano/help/ ./docs/help/*
	@ sudo install -v ./build/cano /usr/bin

uninstall:
	@ rm -rf ~/.local/share/cano
	@ sudo rm -f /usr/bin/cano

.PHONY: clean fclean re
