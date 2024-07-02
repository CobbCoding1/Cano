#ifndef DEFS_H
#define DEFS_H

#include <ncurses.h>
#include "view.h"

#define DATA_START_CAPACITY 1024

#define CRASH(str)                    \
        do {                          \
            frontend_end();                 \
            fprintf(stderr, str"\n"); \
            exit(1);                  \
        } while(0) 

#define ASSERT(cond, ...) \
    do { \
        if (!(cond)) { \
            frontend_end();   \
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
        void *new= calloc(((da)->capacity+1), sizeof(*(da)->data));                    \
        ASSERT(new,"outta ram");                                                       \
        memcpy(new, (da)->data, (da)->count);                                          \
        free((da)->data);                                                              \
        (da)->data = new;                                                              \
    }                                                                                  \
    (da)->data[(da)->count++] = (item);                                                \
} while (0)

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

typedef enum {
    NONE = 0,
    INSERT_CHARS,
    DELETE_CHAR,
    DELETE_MULT_CHAR,
    REPLACE_CHAR,
} Undo_Type;

typedef enum {
    NO_ERROR = 0,
    NOT_ENOUGH_ARGS,
    INVALID_ARGS,
    UNKNOWN_COMMAND,
    INVALID_IDENT,
} Command_Error;

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

// Visual selection range
typedef struct {
    size_t start;
    size_t end;
    int is_line;
} Visual;

// Index in buffer which indicate the start and end of a row
typedef struct {
    size_t start;
    size_t end;
} Row;

typedef struct {
    Row *data;
    size_t count;
    size_t capacity;
} Rows;

// Data is the actual buffer which stores the text
typedef struct {
    char *data;
    size_t count;
    size_t capacity;
} Data;

typedef struct {
    size_t *data;
    size_t count;
    size_t capacity;
} Positions;

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
    Undo_Type type;
    Data data;
    size_t start;
    size_t end;
} Undo;

typedef struct {
    Undo *data;
    size_t count;
    size_t capacity;
} Undo_Stack;
    
typedef struct {
    bool repeating;
    size_t repeating_count;
} Repeating;

typedef struct {
    char *str;
    size_t len;
} Sized_Str;

// For mapping keys
typedef struct {
    int a;
    char *b;
    size_t b_s;
} Map;
    
typedef struct {
    Map *data;
    size_t count;
    size_t capacity;
} Maps;

typedef union {
    int as_int;    
    float as_float;
    void *as_ptr;
} Var_Value;

typedef enum {
    VAR_INT,
    VAR_FLOAT,
    VAR_PTR,
} Var_Type;

typedef struct {
    char *name;
    Var_Value value;    
    Var_Type type;
} Variable;

typedef struct {
    Variable *data;
    size_t count;
    size_t capacity;
} Variables;

typedef struct {
    char *name;
    char *path;
    // Including directories may be useful for making the explorer prettier
    bool is_directory;
} File;

typedef struct {
    File *data;
    size_t count;
    size_t capacity;
} Files;

typedef struct {
    String_View label;
    int *val;
} Config_Vars;

#define CONFIG_VARS 5

typedef struct _Config_ {
    // Config variables
    int relative_nums;
    int auto_indent;
    int syntax;
    int indent;
    int undo_size;
    char *lang;

    // Control variables
    int QUIT;
    Mode mode;

    // Colors
    int background_color; // -1 for terminal background color.

    char leaders[LEADER_COUNT];
    Maps key_maps;

    Config_Vars vars[CONFIG_VARS];
} Config;

typedef struct State {
    Undo_Stack undo_stack;
    Undo_Stack redo_stack;
    Undo cur_undo;
    size_t num_of_braces; // braces that preceed the cursor
    int ch;               // current character
	char *env;	        // home folder
    
    char *command;        // most recent command entered by user
    size_t command_s;     // size of command
    
    Variables variables;
    
    Repeating repeating;
    Data num;
    Leader leader;        // current leader key

    bool is_print_msg;
    char *status_bar_msg;

    // rendering positions
    size_t x;
    size_t y;
    size_t normal_pos;

    // key functions to be called on each keypress
    void(**key_func)(Buffer *buffer, Buffer **modify_buffer, struct State *state);

    Sized_Str clipboard;

    // files for file explorer (ctrl+n)
    Files* files;
    bool is_exploring;
    size_t explore_cursor;
    
    // text buffer
    Buffer* buffer;

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

    Config config;
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

// positions for syntax highlighting
typedef struct {
    size_t row;
    size_t col;
    size_t size;
} Syntax_Highlighting;
    
extern char *string_modes[MODE_COUNT];

#endif // DEFS_H
