#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIEW_IMPLEMENTATION
#include "view.h"

char *keywords[] = {
    "void",
    "int",
    "for",
    "while",
    "return",
};
#define NUM_KEYWORDS sizeof(keywords)/sizeof(*keywords)

typedef enum {
    Type_None = 0,
    Type_Keyword,
} Token_Type;

typedef struct {
    Token_Type type;
    size_t index;
    size_t size;
} Token;

int is_keyword(char *word, size_t word_s) {
    for(size_t i = 0; i < NUM_KEYWORDS; i++) {
        if(word_s < strlen(keywords[i])) continue;
        if(strcmp(word, keywords[i]) == 0) return 1;
    }
    return 0;
}

int is_in_tokens_index(Token *token_arr, size_t token_s, size_t index, size_t *size) {
    for(size_t i = 0; i < token_s; i++) {
        if(token_arr[i].index == index) {
            *size = token_arr[i].size;
            return 1;
        }
    }
    return 0;
}

Token generate_word(String_View *view, char *contents) {
    size_t index = view->data - contents;
    char word[32] = {0};
    size_t word_s = 0;
    while(view->len > 0 && (isalpha(view->data[0]) || view->data[0] == '_')) {
        word[word_s++] = view->data[0]; 
        view->data++;
        view->len--;
    }
    view->data--;
    view->len++;
    if(is_keyword(word, word_s)) {
        return (Token){.type = Type_Keyword, .index = index, .size = word_s};
    }
    return (Token){Type_None};
}

size_t generate_tokens(char *line, size_t line_s, Token *token_arr, size_t *token_arr_capacity) {
    size_t token_arr_s = 0;

    String_View view = view_create(line, line_s);
    while(view.len > 0) {
        view = view_trim_left(view);
        if(isalpha(view.data[0])) {
            Token token = generate_word(&view, line);
            if(token_arr_s >= *token_arr_capacity) {
                token_arr = realloc(token_arr, sizeof(Token)*(*token_arr_capacity)*2);
                *token_arr_capacity *= 2;
            }
            if(token.type != Type_None) {
                token_arr[token_arr_s++] = token;
            }
        }
        view.data++;
        if(view.len == 0) break;
        view.len--;
    }

    return token_arr_s;
}
