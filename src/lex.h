#ifndef LEX_H
#define LEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "frontend.h"
#include "view.h"
#include "colors.h"

typedef enum {
    Type_None = 0,
    Type_Keyword,
    Type_Type,
    Type_Preprocessor,
    Type_String,
    Type_Comment,
    Type_Word,
} Token_Type;

typedef struct {
    Token_Type type;
    size_t index;
    size_t size;
} Token;

int is_keyword(char *word, size_t word_s);
int is_type(char *word, size_t word_s);
char *strip_off_dot(char *str, size_t str_s);
size_t read_file_to_str(char *filename, char **contents);
Color_Arr parse_syntax_file(char *filename);
int is_in_tokens_index(Token *token_arr, size_t token_s, size_t index, size_t *size, Color_Pairs *color);
Token generate_word(String_View *view, char *contents);
size_t generate_tokens(char *line, size_t line_s, Token *token_arr, size_t *token_arr_capacity);
int read_file_by_lines(char *filename, char ***lines, size_t *lines_s);

#endif
