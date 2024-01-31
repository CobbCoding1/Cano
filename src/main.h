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

#include <curses.h>

#include "colors.h"
#include "config.h"

#define DATA_START_CAPACITY 1024

#define CRASH(str)                    \
        do {                          \
            endwin();                 \
            fprintf(stderr, str"\n"); \
            exit(1);                  \
        } while(0) 

#define ASSERT(cond, ...) \
    do { \
        if (!(cond)) { \
            endwin();   \
            fprintf(stderr, "%s:%d: ASSERTION FAILED: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
            exit(1); \
        } \
    } while (0)

#define WRITE_LOG(message, ...)                                                         \
    do {                                                                                \
        FILE *file = fopen("logs/cano.log", "a");                                       \
        if (file != NULL) {                                                             \
            fprintf(file, "%s:%d: " message "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
            fclose(file);                                                               \
        }                                                                               \
    } while(0)


#define DA_APPEND(da, item) do {                                                       \
    if ((da)->count >= (da)->capacity) {                                               \
        (da)->capacity = (da)->capacity == 0 ? DATA_START_CAPACITY : (da)->capacity*2; \
        (da)->data = realloc((da)->data, (da)->capacity*sizeof(*(da)->data));       \
        ASSERT((da)->data != NULL, "outta ram");                               \
    }                                                                                  \
    (da)->data[(da)->count++] = (item);                                               \
} while (0)

#include "lex.c"

#define ctrl(x) ((x) & 0x1f)

#define ESCAPE      27
#define SPACE       32 
#define ENTER       10
#define KEY_TAB     9
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
    LEADER_Y,
    LEADER_COUNT,
} Leader;

char leaders[LEADER_COUNT] = {' ', 'r', 'd', 'y'};

typedef struct {
    const char* path_to_file;
    const char* filename; /* maybe will get used? */
    const char* lang;
} ThreadArgs;

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
    size_t start;
    size_t end;
    int is_line;
} Visual;

#ifdef REFACTOR
typedef struct {
    size_t index;
    size_t size;
    size_t capacity;
    char *contents;
} Row;
#endif

typedef struct {
    size_t start;
    size_t end;
} Row;

typedef struct {
    Row *data;
    size_t count;
    size_t capacity;
} Rows;

typedef struct {
    char *data;
    size_t count;
    size_t capacity;
} Data;

typedef struct {
    Data data;
    Rows rows;
    size_t cursor;
    size_t row;
    size_t col;

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
    char *str;
    size_t len;
} Sized_Str;

typedef struct State {
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

    void(**key_func)(Buffer *buffer, Buffer **modify_buffer, struct State *state);

    Sized_Str clipboard;

    // window sizes
    int grow;
    int gcol;
    int main_row;
    int main_col;
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

int is_between(size_t a, size_t b, size_t c);
char *stringify_mode();
Brace find_opposite_brace(char opening);
Ncurses_Color rgb_to_ncurses(int r, int g, int b);
void init_ncurses_color(int id, int r, int g, int b);
void shift_undo_left(Undo *undo, size_t amount);
void push_undo(Undo *undo, Buffer *buf);
Buffer *pop_undo(Undo *undo);
void reset_command(char *command, size_t *command_s);
Command parse_command(char *command, size_t command_s);
int execute_command(Command *command, Buffer *buf, State *state);
void shift_str_left(char *str, size_t *str_s, size_t index);
void shift_str_right(char *str, size_t *str_s, size_t index);
Buffer *read_file_to_buffer(char *filename);
void buffer_calculate_rows(Buffer *buffer);
int handle_motion_keys(Buffer *buffer, int ch, size_t *repeating_count);
int handle_modifying_keys(Buffer *buffer, State *state);
int handle_normal_to_insert_keys(Buffer *buffer, State *state);
void buffer_insert_char(Buffer *buffer, char ch);
void buffer_delete_char(Buffer *buffer);
void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_command_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_search_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
int main(int argc, char **argv);

#endif
