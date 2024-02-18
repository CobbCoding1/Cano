#include <stdio.h>
#include <stdlib.h>

#define VIEW_IMPLEMENTATION
#include "config.h"

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
    
char *tt_string[TT_COUNT] = {"set_var", "set_output", "set_map", "let", "plus", "minus", 
    "mult", "div", "echo", "w", "e", "we", "ident", "special key", "string", "config var", "int", "float"};

typedef struct {
    Command_Type type;
    String_View value;
    size_t location;
} Command_Token;

typedef struct {
    String_View label;
    int *val;
} Config_Vars;

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


#define CONFIG_VARS 5
Config_Vars vars[CONFIG_VARS] = {
    {{"syntax", sizeof("syntax")-1}, &syntax},
    {{"indent", sizeof("indent")-1}, &indent},
    {{"auto-indent", sizeof("auto-indent")-1}, &auto_indent},
    {{"undo-size", sizeof("undo-size")-1}, &undo_size},
    {{"relative", sizeof("relative")-1}, &relative_nums},
};    
    
String_View view_chop_left(String_View view, size_t amount) {
    if(view.len < amount) {
        view.data += view.len;
        view.len = 0;
        return view;
    }
    
    view.data += amount;
    view.len -= amount;
    return view;
}
    
String_View view_chop_right(String_View view) {
    if(view.len > 0) {
        view.len -= 1;
    }
    return view;
}

    
String_View view_string_internals(String_View view) {
    view = view_chop_left(view, 1);
    view = view_chop_right(view);
    return view;
}
    
Command_Type get_token_type(String_View view) {
    if(isdigit(*view.data)) {
        for(size_t i = 0; i < view.len; i++) {
            if(view.data[i] == ' ') {
                break;
            }
            if(view.data[i] == '.') {
                return TT_FLOAT_LIT;
            }
        }
        return TT_INT_LIT;   
    } else if(*view.data == '"') {
        return TT_STRING;
    } else if(*view.data == '<') {
        return TT_SPECIAL_CHAR;   
    } else if(*view.data == '+') {
        return TT_PLUS;
    } else if(*view.data == '-') {
        return TT_MINUS;
    } else if(*view.data == '*') {
        return TT_MULT;  
    } else if(*view.data == '/') {
        return TT_DIV;  
    } else if(view_cmp(view, LITERAL_CREATE("let"))) {
        return TT_LET;
    } else if(view_cmp(view, LITERAL_CREATE("echo"))) {
        return TT_ECHO;
    } else if(view_cmp(view, LITERAL_CREATE("set-var"))) {
        return TT_SET_VAR;   
    } else if(view_cmp(view, LITERAL_CREATE("w"))) {
        return TT_SAVE;
    } else if(view_cmp(view, LITERAL_CREATE("e"))) {
        return TT_EXIT;
    } else if(view_cmp(view, LITERAL_CREATE("we"))) {
        return TT_SAVE_EXIT;
    } else if(view_cmp(view, LITERAL_CREATE("set-output"))) {
        return TT_SET_OUTPUT;
    } else if(view_cmp(view, LITERAL_CREATE("set-map"))) {
        return TT_SET_MAP;
    } else {
        for(size_t i = 0; i < CONFIG_VARS; i++) {
            if(view_cmp(view, vars[i].label)) {
                 return TT_CONFIG_IDENT;       
            }   
        }
        return TT_IDENT;
    }
}

Command_Token create_token(String_View command) {
    String_View starting = command;
    while(command.len > 0 && *command.data != ' ') {
        if(*command.data == '"') {
            command = view_chop_left(command, 1);
            while(command.len > 0 && *command.data != '"') {
                command = view_chop_left(command, 1);
            }
        }
        if(*command.data == '<') {
            command = view_chop_left(command, 1);
            while(command.len > 0 && *command.data != '>') {
                command = view_chop_left(command, 1);
            }
        }
        command = view_chop_left(command, 1);
    }       
    String_View result = {
        .data = starting.data,
        .len = starting.len-command.len
    };    
    Command_Token token = {.value = result};
    token.type = get_token_type(result);
    return token;
}
    
Command_Token *lex_command(String_View command, size_t *token_s) {
    size_t count = 0;
    for(size_t i = 0; i < command.len; i++) {
        if(command.data[i] == '"') {
            i++;
            while(i < command.len && command.data[i] != '"') {
                i++;
            }
        }
        if(command.data[i] == ' ') {
            count++;
        }
    }
    *token_s = count + 1;
    Command_Token *result = malloc(sizeof(Command_Token)*(*token_s));
    size_t result_s = 0;
    String_View starting = command;
    while(command.len > 0) {
        assert(result_s <= *token_s);
        size_t loc = command.data - starting.data;
        Command_Token token = create_token(command);   
        token.location = loc;
        command = view_chop_left(command, token.value.len+1);
        view_chop_left(command, 1);
        result[result_s++] = token;
    }
    return result;
}
    
void print_token(Command_Token token) {
    printf("location: %zu, type: %d, value: "View_Print"\n", token.location, token.type, View_Arg(token.value));
}

int expect_token(State *state, Command_Token token, Command_Type type) {
    if(token.type != type) {
        sprintf(state->status_bar_msg, "Invalid arg, expected %s but found %s", tt_string[type], tt_string[token.type]);    
        state->is_print_msg = 1;
    }
    return(token.type == type);
}
    
Node *create_node(Node_Type type, Node_Val value) {
    Node *node = malloc(sizeof(Node));
    node->type = type;
    node->value = value;        
    node->left = NULL;
    node->right = NULL;
    return node;
}

Operator get_operator(Command_Token token) {
    switch(token.type) {
        case TT_PLUS:
            return OP_PLUS;
        case TT_MINUS:
            return OP_MINUS;
        case TT_MULT:
            return OP_MULT;
        case TT_DIV:
            return OP_DIV;
        default:
            return OP_NONE;
    }
}

typedef struct {
    char *name;
    int value;
} Ctrl_Key;

Ctrl_Key ctrl_keys[] = {
    {"<ctrl-a>", ctrl('a')},
    {"<ctrl-b>", ctrl('b')},    
    {"<ctrl-c>", ctrl('c')},    
    {"<ctrl-d>", ctrl('d')},
    {"<ctrl-e>", ctrl('e')},    
    {"<ctrl-f>", ctrl('f')},    
    {"<ctrl-g>", ctrl('g')},
    {"<ctrl-h>", ctrl('h')},    
    {"<ctrl-i>", ctrl('i')},    
    {"<ctrl-j>", ctrl('j')},
    {"<ctrl-k>", ctrl('k')},    
    {"<ctrl-l>", ctrl('l')},    
    {"<ctrl-m>", ctrl('m')},
    {"<ctrl-n>", ctrl('n')},    
    {"<ctrl-o>", ctrl('o')},    
    {"<ctrl-p>", ctrl('p')},
    {"<ctrl-q>", ctrl('q')},    
    {"<ctrl-r>", ctrl('r')},    
    {"<ctrl-s>", ctrl('s')},
    {"<ctrl-t>", ctrl('t')},    
    {"<ctrl-u>", ctrl('u')},    
    {"<ctrl-v>", ctrl('v')},
    {"<ctrl-w>", ctrl('w')},    
    {"<ctrl-x>", ctrl('x')},    
    {"<ctrl-y>", ctrl('y')},
    {"<ctrl-z>", ctrl('z')},    
};
#define NUM_OF_CTRL_KEYS sizeof(ctrl_keys)/sizeof(*ctrl_keys)
    
int get_special_char(String_View view) {
    if(view_cmp(view, LITERAL_CREATE("<space>"))) {
        return SPACE;
    } else if(view_cmp(view, LITERAL_CREATE("<esc>"))) {
        return ESCAPE;
    } else if(view_cmp(view, LITERAL_CREATE("<backspace>"))) {
        return KEY_BACKSPACE;
    } else if(view_cmp(view, LITERAL_CREATE("<enter>"))) {
        return ENTER;
    } else {
        for(size_t i = 0; i < NUM_OF_CTRL_KEYS; i++) {
            if(view_cmp(view, view_create(ctrl_keys[i].name, strlen(ctrl_keys[i].name)))) {
                return ctrl_keys[i].value;        
            }          
        }
        return -1;
    }
}
    
Bin_Expr *parse_bin_expr(State *state, Command_Token *command, size_t command_s) {
    if(command_s == 0) return NULL;
    Bin_Expr *expr = calloc(0, sizeof(Bin_Expr));
    expr->lvalue = (Expr){view_to_int(command[0].value)};
    if(command_s <= 2) return expr;
    expr->operator = get_operator(command[1]);        
        
    if(expr->operator == OP_NONE) return NULL;
    if(!expect_token(state, command[2], TT_INT_LIT)) return NULL;
    
    expr->rvalue = (Expr){view_to_int(command[2].value)};        
    
    if(command_s > 3) {
        expr->right = parse_bin_expr(state, command+4, command_s-4);
        expr->right->operator = get_operator(command[3]);
    }
    return expr;
}
    
Node *parse_command(State *state, Command_Token *command, size_t command_s) {
    Node *root = NULL;
    Node_Val val;
    val.as_keyword = command[0].type;
    root = create_node(NODE_KEYWORD, val);    
    switch(command[0].type) {
        case TT_SET_VAR:
            if(command_s > 3) {
                sprintf(state->status_bar_msg, "Too many args");
                state->is_print_msg = 1;
                return NULL;
            } else if(command_s < 3) {
                sprintf(state->status_bar_msg, "Not enough args");
                state->is_print_msg = 1;
                return NULL;
            }
        
            if(!expect_token(state, command[1], TT_CONFIG_IDENT) || !expect_token(state, command[2], TT_INT_LIT)) return NULL;
        
            for(size_t i = 0; i < CONFIG_VARS; i++) {
                if(view_cmp(command[1].value, vars[i].label)) {
                    val.as_config = &vars[i];
                }
            }            

            Node *left = create_node(NODE_CONFIG, val);
            root->left = left;
            if(command_s == 3) {
                int value = view_to_int(command[2].value);
                val.as_expr = (Expr){.value = value};
                Node *right = create_node(NODE_EXPR, val);
                root->right = right;
                break;
            } else {
                Bin_Expr *expr = parse_bin_expr(state, command+2, command_s-2);
                val.as_bin = *expr;
                root->right = create_node(NODE_BIN, val);
            }
            break;
        case TT_SET_OUTPUT:
            if(command_s < 2) {
                sprintf(state->status_bar_msg, "Not enough args");
                state->is_print_msg = 1;
                return NULL;
            } else if(command_s > 2) {
                sprintf(state->status_bar_msg, "Too many args");
                state->is_print_msg = 1;
                return NULL;
            }
            val.as_str = (Str_Literal){.value = view_string_internals(command[1].value)};
            root->right = create_node(NODE_STR, val);
            break;
        case TT_SET_MAP:
            if(command_s < 3) {
                sprintf(state->status_bar_msg, "Not enough args");
                state->is_print_msg = 1;
                return NULL;
            } else if(command_s > 3) {
                sprintf(state->status_bar_msg, "Not enough args");
                state->is_print_msg = 1;
                return NULL;
            }
            if(!expect_token(state, command[2], TT_STRING)) return NULL;
            if(command[1].type == TT_SPECIAL_CHAR) {
                int special = get_special_char(command[1].value);
                if(special == -1) {
                    sprintf(state->status_bar_msg, "Invalid special key");
                    state->is_print_msg = 1;
                    return NULL;
                }
                val.as_int = special;
                root->left = create_node(NODE_INT, val); 
            } else {
                val.as_ident = (Identifier){.name = command[1].value};            
                root->left = create_node(NODE_IDENT, val);            
            }
            val.as_str = (Str_Literal){view_string_internals(command[2].value)};
            root->right = create_node(NODE_STR, val);
            break;
        case TT_LET:
            if(!expect_token(state, command[1], TT_IDENT) || !expect_token(state, command[2], TT_INT_LIT)) return NULL;
            val.as_ident = (Identifier){.name = command[1].value};
            root->left = create_node(NODE_IDENT, val);
            if(command_s == 3) {
                int value = view_to_int(command[2].value);
                val.as_expr = (Expr){.value = value};
                Node *right = create_node(NODE_EXPR, val);
                root->right = right;
                break;
            } else {
                Bin_Expr *expr = parse_bin_expr(state, command+2, command_s-2);
                val.as_bin = *expr;
                root->right = create_node(NODE_BIN, val);
            }
            break;
        case TT_ECHO:
            if(command_s > 2) {
                sprintf(state->status_bar_msg, "Too many args");
                state->is_print_msg = 1;
                return NULL;
            }
            if(command_s < 2) {
                sprintf(state->status_bar_msg, "Not enough args");
                state->is_print_msg = 1;
                return NULL;
            }
        
            if(!expect_token(state, command[1], TT_IDENT) && !expect_token(state, command[1], TT_STRING)) return NULL;
            if(command[1].type == TT_STRING) {
                val.as_str = (Str_Literal){view_string_internals(command[1].value)};
                root->right = create_node(NODE_STR, val);
            } else {
                val.as_ident = (Identifier){.name = command[1].value};
                root->right = create_node(NODE_IDENT, val);
            }
            break;
        case TT_PLUS:
        case TT_MINUS:
        case TT_MULT:
        case TT_DIV:
        case TT_SAVE:
        case TT_EXIT:
        case TT_SAVE_EXIT:
        case TT_IDENT:
        case TT_STRING:
        case TT_CONFIG_IDENT:
        case TT_INT_LIT:
        case TT_SPECIAL_CHAR:
        case TT_FLOAT_LIT:
            break;
        case TT_COUNT:
            assert(0 && "UNREACHABLE");
    }
    return root;
}
    
int interpret_expr(Bin_Expr *expr) {
    int value = expr->lvalue.value;
    if(expr->rvalue.value != 0 && expr->operator != OP_NONE) {
        switch(expr->operator) {
            case OP_PLUS:
                value += expr->rvalue.value;
                break;
            case OP_MINUS:
                value -= expr->rvalue.value;
                break;
            case OP_MULT:
                value *= expr->rvalue.value;            
                break;
            case OP_DIV:
                value /= expr->rvalue.value;                        
                break;
            default:
                assert(0 && "unreachable");
        }
    }
    
    if(expr->right != NULL) {
        switch(expr->right->operator) {
            case OP_PLUS:
                value += interpret_expr(expr->right);                                                
                break;
            case OP_MINUS:
                value -= interpret_expr(expr->right);                                    
                break;
            case OP_MULT:
                value *= interpret_expr(expr->right);                        
                break;
            case OP_DIV:
                value /= interpret_expr(expr->right);                        
                break;
            default:
                assert(0 && "unreachable");
        }
    }
    return value;
}

void interpret_command(Buffer *buffer, State *state, Node *root) {
    if(root == NULL) return;
 
    switch(root->type) {
        case NODE_EXPR:
            break;
        case NODE_BIN:
            break;
        case NODE_KEYWORD:
            switch(root->value.as_keyword) {
                case TT_SET_VAR: {
                    Config_Vars *var = root->left->value.as_config;                    
                    if(root->right->type == NODE_EXPR) {
                        *var->val = root->right->value.as_expr.value;
                    } else {
                        Node *node = root->right;
                        int value = interpret_expr(&node->value.as_bin);
                        *var->val = value;
                    }
                    return;
                } break;
                case TT_SET_OUTPUT:
                    buffer->filename = view_to_cstr(root->right->value.as_str.value);
                    break;
                case TT_SET_MAP: {
                        char *str = view_to_cstr(root->right->value.as_str.value);
                        Map map = {.b = str, .b_s = root->right->value.as_str.value.len+1};
                        if(root->left->type == NODE_IDENT) {
                            map.a = root->left->value.as_ident.name.data[0];
                        } else {
                            map.a = root->left->value.as_int;
                        }
                        DA_APPEND(&key_maps, map);
                } break;
                case TT_LET: {
                    Variable var = {0};
                    var.name = view_to_cstr(root->left->value.as_ident.name);
                    var.type = VAR_INT;
                    if(root->right->type == NODE_EXPR) {
                        var.value.as_int = root->right->value.as_expr.value;
                    } else {
                        Node *node = root->right;
                        int value = interpret_expr(&node->value.as_bin);
                        var.value.as_int = value;
                    }
                    DA_APPEND(&state->variables, var);
                } break;
                case TT_ECHO:
                    state->is_print_msg = 1;
                    if(root->right->type == NODE_STR) {
                        String_View str = root->right->value.as_str.value;
                        state->status_bar_msg = view_to_cstr(str);
                    } else {
                        for(size_t i = 0; i < state->variables.count; i++) {
                            String_View var = view_create(state->variables.data[i].name, strlen(state->variables.data[i].name));
                            if(view_cmp(root->right->value.as_ident.name, var)) {
                                if(state->variables.data[i].type == VAR_INT) sprintf(state->status_bar_msg, "%d", state->variables.data[i].value.as_int);
                                else if(state->variables.data[i].type == VAR_FLOAT) sprintf(state->status_bar_msg, "%f", state->variables.data[i].value.as_float);
                                return;
                            }
                        }
                    }
                    break;
                case TT_SAVE:
                    handle_save(buffer);
                    break;
                case TT_EXIT:
                    QUIT = 1;
                    break;
                case TT_SAVE_EXIT:
                    handle_save(buffer);
                    QUIT = 1;
                    break;
                default:
                    return;
            }
            break;
        case NODE_STR:
        case NODE_IDENT:
        case NODE_INT:
        case NODE_CONFIG:
            break;
    }
    
    interpret_command(buffer, state, root->left);   
    interpret_command(buffer, state, root->right);   
}
    
void print_tree(Node *node, size_t depth) {
    if(node == NULL) return;
    if(node->right != NULL) WRITE_LOG("NODE->RIGHT");
    if(node->left != NULL) WRITE_LOG("NODE->LEFT");
    switch(node->type) {
    case NODE_EXPR: 
        WRITE_LOG("EXPR");
        break;
    case NODE_BIN: 
        WRITE_LOG("BIN");
        break;
    case NODE_KEYWORD: 
        WRITE_LOG("KEYWORD");
        break;
    case NODE_STR: 
        WRITE_LOG("STR");
        break;
    case NODE_IDENT: 
        WRITE_LOG("IDENT");
        break;
    case NODE_CONFIG: 
        WRITE_LOG("CONFIG");
        break;
    case NODE_INT:
        WRITE_LOG("INT");
        break;
    }
    print_tree(node->left, depth+1);
    print_tree(node->right, depth+1);    
}

int execute_command(Buffer *buffer, State *state, Command_Token *command, size_t command_s) {
    assert(command_s > 0);
    Node *root = parse_command(state, command, command_s);
    if(root == NULL) return 1;
    print_tree(root, 0);
    interpret_command(buffer, state, root);
    return 0;
}