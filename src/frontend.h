#ifndef FRONTEND_H
#define FRONTEND_H

#include <ncurses.h>
#include <stddef.h>

#include "defs.h"

void frontend_init(State *state);
void state_render(State *state);
void frontend_resize_window(State *state);
int frontend_getch(WINDOW *window);
void frontend_move_cursor(WINDOW *window, size_t pos_x, size_t pos_y);
void frontend_cursor_visible(int value);
void frontend_end(void);

#endif // FRONTEND_H
