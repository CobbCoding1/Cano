#ifndef MAIN_H
#define MAIN_H

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
#include <locale.h>

#include "cgetopt.h"
#include "defs.h"
#include "colors.h"
#include "lex.h"
#include "commands.h"
#include "frontend.h"
#include "keys.h"
#include "tools.h"
#include "buffer.h"

#define CREATE_UNDO(t, p)    \
    Undo undo = {0};         \
    undo.type = (t);         \
    undo.start = (p);        \
    state->cur_undo = undo   \

/* --------------------------- FUNCTIONS --------------------------- */

int main(int argc, char **argv);

#endif // MAIN_H
