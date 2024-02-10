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
    TT_STRING,
    TT_CONFIG_IDENT,
    TT_INT_LIT,
    TT_FLOAT_LIT,
} Command_Type;

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
} Node_Val;

typedef enum {
    NODE_EXPR,
    NODE_BIN,
    NODE_KEYWORD,
    NODE_STR,
    NODE_IDENT,
    NODE_CONFIG,
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

int expect_token(Command_Token token, Command_Type type) {
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
    
Bin_Expr *parse_bin_expr(Command_Token *command, size_t command_s) {
    Bin_Expr *expr = malloc(sizeof(Bin_Expr));
    expr->lvalue = (Expr){view_to_int(command[0].value)};
    if(!expect_token(command[1], TT_PLUS)) return NULL;
    if(!expect_token(command[2], TT_INT_LIT)) return NULL;
    if(command_s == 3) {
        expr->rvalue = (Expr){view_to_int(command[2].value)};
    } else {
        expr->right = parse_bin_expr(command+2, command_s-2);
    }
    return expr;
}
    
Node *parse_command(Command_Token *command, size_t command_s) {
    Node *root = NULL;
    switch(command[0].type) {
        case TT_SET_VAR:
            if(!expect_token(command[1], TT_CONFIG_IDENT)) return NULL;
            if(!expect_token(command[2], TT_INT_LIT)) return NULL;
            Node_Val val;
            val.as_keyword = command[0].type;
            root = create_node(NODE_KEYWORD, val);
        
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
                Bin_Expr *expr = parse_bin_expr(command+2, command_s-2);
                val.as_bin = *expr;
                root->right = create_node(NODE_BIN, val);
            }
            break;
        case TT_SET_OUTPUT:
            break;
        case TT_SET_MAP:
            break;
        case TT_LET:
            break;
        case TT_PLUS:
            break;
        case TT_MINUS:
            break;
        case TT_MULT:
            break;
        case TT_DIV:
            break;
        case TT_ECHO:
            break;
        case TT_SAVE:
            break;
        case TT_EXIT:
            break;
        case TT_SAVE_EXIT:
            break;
        case TT_IDENT:
            break;
        case TT_STRING:
            break;
        case TT_CONFIG_IDENT:
            break;
        case TT_INT_LIT:
            break;
        case TT_FLOAT_LIT:
            break;
    }
    return root;
}

int interpret_expr(Bin_Expr *expr) {
    int value = expr->lvalue.value;    
    if(expr->right == NULL) {
        value += expr->rvalue.value;
    } else {
        value += interpret_expr(expr->right);
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
                    break;
                case TT_SET_MAP:
                    break;
                case TT_LET:
                    break;
                case TT_PLUS:
                    break;
                case TT_MINUS:
                    break;
                case TT_MULT:
                    break;
                case TT_DIV:
                    break;
                case TT_ECHO:
                    break;
                case TT_SAVE:
                    break;
                case TT_EXIT:
                    break;
                case TT_SAVE_EXIT:
                    break;
                case TT_IDENT:
                    break;
                case TT_STRING:
                    break;
                case TT_CONFIG_IDENT:
                    break;
                case TT_INT_LIT:
                    break;
                case TT_FLOAT_LIT:
                    break;
            }
            break;
        case NODE_STR:
            break;
        case NODE_IDENT:
            break;
        case NODE_CONFIG:
            break;
    }
    
    interpret_command(buffer, state, root->left);   
    interpret_command(buffer, state, root->right);   
}
    
void print_tree(Node *node) {
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

    }
    print_tree(node->left);
    print_tree(node->right);    
}

Command_Error execute_command(Buffer *buffer, State *state, Command_Token *command, size_t command_s) {
    (void)state;
    assert(command_s > 0);
    Node *root = parse_command(command, command_s);
    print_tree(root);
    interpret_command(buffer, state, root);
    return NO_ERROR;
    switch(command[0].type) {
        case TT_SET_VAR:
            if(command_s != 3) return NOT_ENOUGH_ARGS;
            if(!expect_token(command[1], TT_CONFIG_IDENT)) return INVALID_ARGS;
            if(!expect_token(command[2], TT_INT_LIT)) return INVALID_ARGS;
            int value = view_to_int(command[2].value);
            for(size_t i = 0; i < CONFIG_VARS; i++) {
                if(view_cmp(command[1].value, vars[i].label)) {
                    *vars[i].val = value;
                }
            }            
            break;   
        case TT_SET_OUTPUT:
            if(command_s != 2) return NOT_ENOUGH_ARGS;
            buffer->filename = view_to_cstr(command[1].value);
            break;
        case TT_SET_MAP:
            if(command_s != 3) return NOT_ENOUGH_ARGS;
            if(!expect_token(command[1], TT_IDENT)) return INVALID_ARGS;
            if(!expect_token(command[2], TT_STRING)) return INVALID_ARGS;
            String_View str = view_string_internals(command[2].value);
            char *str_str = view_to_cstr(str);
            Map map = (Map){.a = command[1].value.data[0], .b = str_str, .b_s = str.len+1};
            DA_APPEND(&key_maps, map);
            break;
        case TT_LET: {
            if(command_s != 3) return NOT_ENOUGH_ARGS;
            if(!expect_token(command[1], TT_IDENT)) return INVALID_ARGS;
            if(!expect_token(command[2], TT_INT_LIT) && !expect_token(command[2], TT_FLOAT_LIT)) return INVALID_ARGS;
            Variable var = {0};
            var.name = view_to_cstr(command[1].value);
            if(command[2].type == TT_INT_LIT) {
                var.type = VAR_INT;
                var.value.as_int = view_to_int(command[2].value);   
            } else {
                var.type = VAR_FLOAT;
                var.value.as_float = view_to_float(command[2].value);   
                WRITE_LOG("%f", var.value.as_float);
            }
            DA_APPEND(&state->variables, var);
        } break;
        case TT_ECHO: {
            if(command_s != 2) return NOT_ENOUGH_ARGS;
            if(!expect_token(command[1], TT_IDENT) && !expect_token(command[1], TT_STRING)) return INVALID_ARGS;
            state->is_print_msg = 1;
            if(command[1].type == TT_STRING) {
                String_View str = view_string_internals(command[1].value);
                state->status_bar_msg = view_to_cstr(str);
            } else {
                for(size_t i = 0; i < state->variables.count; i++) {
                    String_View var = view_create(state->variables.data[i].name, strlen(state->variables.data[i].name));
                    if(view_cmp(command[1].value, var)) {
                       if(state->variables.data[i].type == VAR_INT) sprintf(state->status_bar_msg, "%d", state->variables.data[i].value.as_int);
                       else if(state->variables.data[i].type == VAR_FLOAT) sprintf(state->status_bar_msg, "%f", state->variables.data[i].value.as_float);
                       return NO_ERROR;
                    }
                }
                return INVALID_IDENT;
            }
        } break;
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
        case TT_CONFIG_IDENT:
        case TT_INT_LIT:
        default:
            return UNKNOWN_COMMAND;
            break;
    }
    return NO_ERROR;
}
