#ifndef FRONTEND_H
#define FRONTEND_H

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

#include "defs.h"
#include "lex.h"

void frontend_init(State *state);
void state_render(State *state);
void frontend_resize_window(State *state);
int frontend_getch(WINDOW *window);
void frontend_move_cursor(WINDOW *window, size_t pos_x, size_t pos_y);
void frontend_cursor_visible(int value);
void frontend_end();

#endif // FRONTEND_H