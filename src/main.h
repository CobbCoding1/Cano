#ifndef MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#include "colors.h"
#include "config.h"
#include "lex.c"
#include "commands.c"

#define DATA_START_CAPACITY 1024

#define CREATE_UNDO(t, p)    \
    Undo undo = {0};         \
    undo.type = (t);         \
    undo.start = (p);        \
    state->cur_undo = undo   \

/* --------------------------- FUNCTIONS --------------------------- */

int is_between(size_t a, size_t b, size_t c);
char *stringify_mode();
Brace find_opposite_brace(char opening);
Ncurses_Color rgb_to_ncurses(int r, int g, int b);
void init_ncurses_color(int id, int r, int g, int b);
void shift_undo_left(Undo *undo, size_t amount);
void reset_command(char *command, size_t *command_s);
void shift_str_left(char *str, size_t *str_s, size_t index);
void shift_str_right(char *str, size_t *str_s, size_t index);
Buffer *read_file_to_buffer(char *filename);
void buffer_calculate_rows(Buffer *buffer);
int handle_motion_keys(Buffer *buffer, State *state, int ch, size_t *repeating_count);
int handle_modifying_keys(Buffer *buffer, State *state);
int handle_normal_to_insert_keys(Buffer *buffer, State *state);
void buffer_insert_char(Buffer *buffer, char ch);
void buffer_delete_char(Buffer *buffer, State *state);
void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_command_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_search_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
int main(int argc, char **argv);

#endif
