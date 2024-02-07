#ifndef CONFIG_H
#define CONFIG_H

#include "defs.h"

// Config variables
int relative_nums = 1;
int auto_indent = 1;
int syntax = 1;
int indent = 4;
int undo_size = 16;
char *lang = " ";

// Control variables
int QUIT = 0;
Mode mode = NORMAL;

#endif
