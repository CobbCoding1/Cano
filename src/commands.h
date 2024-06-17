#ifndef COMMANDS_H
#define COMMANDS_H

#include "defs.h"

typedef enum {
    TT_SET_VAR,
    TT_SET_OUTPUT,
    TT_SET_MAP,
    TT_LET,
    TT_PLUS,
    TT_MINUS,
    TT_MULT,
    TT_DIV,
    TT_ECHO,
    TT_SAVE,
    TT_EXIT,
    TT_SAVE_EXIT,
    TT_IDENT,
    TT_SPECIAL_CHAR,
    TT_STRING,
    TT_CONFIG_IDENT,
    TT_INT_LIT,
    TT_FLOAT_LIT,
    TT_COUNT,
} Command_Type;

typedef struct {
    Command_Type type;
    String_View value;
    size_t location;
} Command_Token;

typedef struct {
    String_View name;
    int value;
} Identifier;

typedef struct {
    String_View value;
} Str_Literal;

typedef struct {
    int value;
} Expr;
    
typedef enum {
    OP_NONE = 0,
    OP_PLUS,
    OP_MINUS,
    OP_MULT,
    OP_DIV,
} Operator;

typedef struct Bin_Expr {
    Expr lvalue;
    struct Bin_Expr *right;
    Expr rvalue;
    Operator operator;
} Bin_Expr;
    
typedef union {
    Expr as_expr;
    Bin_Expr as_bin;   
    Command_Type as_keyword;
    Str_Literal as_str;
    Identifier as_ident;
    Config_Vars *as_config;
    int as_int;
} Node_Val;

typedef enum {
    NODE_EXPR,
    NODE_BIN,
    NODE_KEYWORD,
    NODE_STR,
    NODE_IDENT,
    NODE_CONFIG,
    NODE_INT,
} Node_Type;
 
typedef struct Node {
    Node_Val value;
    Node_Type type;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    char *name;
    int value;
} Ctrl_Key;

Command_Type get_token_type(State *state, String_View view);
Command_Token create_token(State *state, String_View command);
Command_Token *lex_command(State *state, String_View command, size_t *token_s);   
void print_token(Command_Token token);
int expect_token(State *state, Command_Token token, Command_Type type);   
Node *create_node(Node_Type type, Node_Val value);
Operator get_operator(Command_Token token);
int get_special_char(String_View view);   
Bin_Expr *parse_bin_expr(State *state, Command_Token *command, size_t command_s);   
Node *parse_command(State *state, Command_Token *command, size_t command_s);   
int interpret_expr(Bin_Expr *expr);
void interpret_command(Buffer *buffer, State *state, Node *root);   
void print_tree(Node *node, size_t depth);
int execute_command(Buffer *buffer, State *state, Command_Token *command, size_t command_s);

#endif
