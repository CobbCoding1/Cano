#include <stdio.h>
#include <stdlib.h>

#define VIEW_IMPLEMENTATION
#include "config.h"

typedef enum {
    TT_SET_VAR,
    TT_SET_OUTPUT,
    TT_EXIT,
    TT_SAVE_EXIT,
    TT_CONFIG_IDENT,
    TT_INT_LIT,
} Command_Type;

typedef struct {
    Command_Type type;
    String_View value;
    size_t location;
} Command_Token;
    
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
    
Command_Type get_token_type(String_View view) {
    if(isdigit(*view.data)) {
        return TT_INT_LIT;   
    } else if(view_cmp(view, LITERAL_CREATE("set-var"))) {
        return TT_SET_VAR;   
    } else if(view_cmp(view, LITERAL_CREATE("e"))) {
        return TT_EXIT;
    } else if(view_cmp(view, LITERAL_CREATE("we"))) {
        return TT_SAVE_EXIT;
    } else if(view_cmp(view, LITERAL_CREATE("set-output"))) {
        return TT_SET_OUTPUT;
    } else {
        return TT_CONFIG_IDENT;
    }
}

Command_Token create_token(String_View command) {
    String_View starting = command;
    while(command.len > 0 && *command.data != ' ') {
        command = view_chop_left(command, 1);
    }       
    String_View result = {
        .data = starting.data,
        .len = starting.len-command.len
    };    
    return (Command_Token){
        .value = result,
        .type = get_token_type(result),
    };
}
    
Command_Token *lex_command(String_View command, size_t *token_s) {
    size_t count = 0;
    for(size_t i = 0; i < command.len; i++) {
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

void expect_token(Command_Token token, Command_Type type) {
    if(token.type != type) {
        CRASH("Incorrect token type");
    }
}

typedef struct {
    String_View label;
    int *val;
} Config_Vars;

#define CONFIG_VARS 5
Config_Vars vars[CONFIG_VARS] = {
    {{"syntax", sizeof("syntax")-1}, &syntax},
    {{"indent", sizeof("indent")-1}, &indent},
    {{"auto-indent", sizeof("auto-indent")-1}, &auto_indent},
    {{"undo-size", sizeof("undo-size")-1}, &undo_size},
    {{"relative", sizeof("relative")-1}, &relative_nums},
};    

int execute_command(Buffer *buffer, State *state, Command_Token *command, size_t command_s) {
    (void)state;
    assert(command_s > 0);
    switch(command[0].type) {
        case TT_SET_VAR:
            if(command_s != 3) return 1;
            expect_token(command[1], TT_CONFIG_IDENT);
            expect_token(command[2], TT_INT_LIT);
            int value = view_to_int(command[2].value);
            for(size_t i = 0; i < CONFIG_VARS; i++) {
                if(view_cmp(command[1].value, vars[i].label)) {
                    *vars[i].val = value;
                }
            }            
            break;   
        case TT_SET_OUTPUT:
            if(command_s != 2) return 1;
            buffer->filename = view_to_cstr(command[1].value);
            break;
        case TT_EXIT:
            QUIT = 1;
            break;
        case TT_SAVE_EXIT:
            handle_save(buffer);
            QUIT = 1;
            break;
        case TT_CONFIG_IDENT:
            break;
        case TT_INT_LIT:
            break;
    }
    return 0;
}
