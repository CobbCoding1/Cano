#ifndef TOOLS_H
#define TOOLS_H

#include "defs.h"
#include "colors.h"
#include "lex.h"

Data dynstr_to_data(Sized_Str str);
void handle_cursor_shape(State *state);
void free_buffer(Buffer *buffer);
void free_undo(Undo *undo);
void free_undo_stack(Undo_Stack *undo);
void handle_save(Buffer *buffer);
Buffer *load_buffer_from_file(char *filename);
void shift_str_left(char *str, size_t *str_s, size_t index);
void shift_str_right(char *str, size_t *str_s, size_t index);
void undo_push(State *state, Undo_Stack *stack, Undo undo);
Undo undo_pop(Undo_Stack *stack);
Brace find_opposite_brace(char opening);
int check_keymaps(Buffer *buffer, State *state);
void scan_files(State *state, char *directory);
void free_files(Files **files);
void load_config_from_file(State *state, Buffer *buffer, char *config_filename, char *syntax_filename);
int contains_c_extension(const char *str);
void *check_for_errors(void *args);
Ncurses_Color rgb_to_ncurses(int r, int g, int b);
void init_ncurses_color(int id, int r, int g, int b);
void reset_command(char *command, size_t *command_s);

#endif // TOOLS_H