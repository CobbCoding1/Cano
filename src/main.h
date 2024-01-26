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

#include <curses.h>

#include "colors.h"
#include "config.h"
#include "lex.c"

#define CRASH(str)                    \
        do {                          \
            endwin();                 \
            fprintf(stderr, str"\n"); \
            exit(1);                  \
        } while(0) 

#define WRITE_LOG(message, ...)                                                         \
    do {                                                                                \
        FILE *file = fopen("logs/cano.log", "a");                                       \
        if (file != NULL) {                                                             \
            fprintf(file, "%s:%d: " message "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
            fclose(file);                                                               \
        }                                                                               \
    } while(0)


#define ctrl(x) ((x) & 0x1f)

#define ESCAPE      27
#define SPACE       32 
#define ENTER       10
#define DOWN_ARROW  258 
#define UP_ARROW    259 
#define LEFT_ARROW  260 
#define RIGHT_ARROW 261 

#define STARTING_ROWS_SIZE 128
#define STARTING_ROW_SIZE 64 

typedef enum {
    NORMAL,
    INSERT,
    SEARCH,
    COMMAND,
    VISUAL,
    MODE_COUNT,
} Mode;
    
typedef enum {
    LEADER_NONE,
    LEADER_R,
    LEADER_D,
} Leader;


typedef struct {
    size_t index;
    size_t size;
    size_t capacity;
    char *contents;
} Row;


typedef struct {
    char color_name[20];
    bool is_custom_line_row;
    bool is_custom;
    int slot;
    int id;
    int red;
    int green;
    int blue;
} Color;


typedef struct {
    size_t x;
    size_t y;
} Point;

typedef struct {
    Point starting_pos;
    Point ending_pos;
    int is_line;
} Visual;

typedef struct {
    Row *rows;
    size_t row_capacity;
    size_t row_index;
    size_t cur_pos;
    size_t row_s;
    char *filename;
    Visual visual;
} Buffer;

typedef struct {
    size_t size;
    char *arg;
} Arg;

typedef struct {
    char *command;
    size_t command_s;
    Arg args[16];
    size_t args_s;
} Command;

typedef struct {
    Buffer **buf_stack;
    size_t buf_stack_s;
    size_t buf_capacity;
} Undo;
    
typedef struct {
    int repeating;
    size_t repeating_count;
} Repeating;

typedef struct {
    Undo undo_stack;
    Undo redo_stack;
    size_t num_of_braces;
    int ch;
    
    char *command;
    size_t command_s;
    
    Repeating repeating;
    Leader leader;

    int is_print_msg;
    char *status_bar_msg;

    size_t x;
    size_t y;
    size_t normal_pos;

    // window sizes
    int main_row;
    int main_col;
    int grow;
    int gcol;
    int line_num_row;
    int line_num_col;
    int status_bar_row;
    int status_bar_col;

    // windows
    WINDOW *line_num_win;
    WINDOW *main_win;
    WINDOW *status_bar;
} State;

typedef struct {
    char brace;
    int closing;
} Brace;


typedef struct {
    int r;
    int g;
    int b;
} Ncurses_Color;

typedef enum {
    NO_ERROR,
    UNKNOWN_COMMAND,
    INVALID_ARG,
    INVALID_VALUE,
} Command_Errors;

typedef struct {
    size_t row;
    size_t col;
    size_t size;
} Syntax_Highlighting;

// global state variables
Mode mode = NORMAL;
int QUIT = 0;

/* --------------------------- FUNCTIONS --------------------------- */

int is_between(Point a, Point b, Point c);
char *stringify_mode();
Brace find_opposite_brace(char opening);
Ncurses_Color rgb_to_ncurses(int r, int g, int b);
void init_ncurses_color(int id, int r, int g, int b);
void free_buffer(Buffer **buffer);
Buffer *copy_buffer(Buffer *buffer);
void shift_undo_left(Undo *undo, size_t amount);
void push_undo(Undo *undo, Buffer *buf);
Buffer *pop_undo(Undo *undo);
void resize_rows(Buffer *buffer, size_t capacity);
void resize_row(Row **row, size_t capacity);
void insert_char(Row *row, size_t pos, char c);
Point search(Buffer *buffer, char *command, size_t command_s);
void replace(Buffer *buffer, Point position, char *new_str, size_t old_str_s, size_t new_str_s);
void find_and_replace(Buffer *buffer, char *old_str, char *new_str);
size_t num_of_open_braces(Buffer *buffer);
void reset_command(char *command, size_t *command_s);
void handle_save(Buffer *buffer);
Command parse_command(char *command, size_t command_s);
int execute_command(Command *command, Buffer *buf, State *state);
void shift_rows_left(Buffer *buf, size_t index);
void shift_rows_right(Buffer *buf, size_t index);
void shift_row_left(Row *row, size_t index);
void shift_row_right(Row *row, size_t index);
void delete_char(Buffer *buffer, size_t row, size_t col, size_t *y, WINDOW *main_win);
void delete_row(Buffer *buffer, size_t row);
void shift_str_left(char *str, size_t *str_s, size_t index);
void shift_str_right(char *str, size_t *str_s, size_t index);
void append_rows(Row *a, Row *b);
void delete_and_append_row(Buffer *buf, size_t index);
void create_and_cut_row(Buffer *buf, size_t dest_index, size_t *str_s, size_t index);
void create_newline_indent(Buffer *buffer, size_t num_of_braces);
Buffer *read_file_to_buffer(char *filename);
int handle_motion_keys(Buffer *buffer, int ch, size_t *repeating_count);
int handle_modifying_keys(Buffer *buffer, State *state, int ch, WINDOW *main_win, size_t *y);
int handle_normal_to_insert_keys(Buffer *buffer, State *state, int ch);
void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_command_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_search_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
int main(int argc, char **argv);

#endif
