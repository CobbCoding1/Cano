#ifndef BUFFER_H
#define BUFFER_H

#include "defs.h"
#include "tools.h"
#include "frontend.h"

void buffer_calculate_rows(Buffer *buffer);
void buffer_insert_char(State *state, Buffer *buffer, char ch);
void buffer_delete_char(Buffer *buffer, State *state);
size_t buffer_get_row(const Buffer *buffer);
size_t index_get_row(Buffer *buffer, size_t index);
void buffer_yank_line(Buffer *buffer, State *state, size_t offset);
void buffer_yank_char(Buffer *buffer, State *state);
void buffer_yank_selection(Buffer *buffer, State *state, size_t start, size_t end);
void buffer_delete_selection(Buffer *buffer, State *state, size_t start, size_t end);
void buffer_insert_selection(Buffer *buffer, Data *selection, size_t start);
void buffer_move_up(Buffer *buffer);
void buffer_move_down(Buffer *buffer);
void buffer_move_right(Buffer *buffer);
void buffer_move_left(Buffer *buffer);
int skip_to_char(Buffer *buffer, int cur_pos, int direction, char c);
void buffer_next_brace(Buffer *buffer);
int isword(char ch);
void buffer_create_indent(Buffer *buffer, State *state);
void buffer_newline_indent(Buffer *buffer, State *state);
State init_state();

#endif