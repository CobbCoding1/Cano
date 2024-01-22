#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define VIEW_IMPLEMENTATION
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

char *types_old[] = {
    "char",
    "double",
    "float",
    "int",
    "long",
    "short",
    "void",
    "size_t",
};

char *keywords_old[] = {
    "auto",
    "break",
    "case",
    "const",
    "continue",
    "default",
    "do",
    "else",
    "enum",
    "extern",
    "for",
    "goto",
    "if",
    "register",
    "return",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "volatile",
    "while",
};

char **keywords;
size_t keywords_s = 0;

char **types;
size_t types_s = 0;

char *comments = "//";

#define NUM_KEYWORDS sizeof(keywords)/sizeof(*keywords)
#define NUM_TYPES sizeof(types)/sizeof(*types)

typedef struct {
    Token_Type type;
    size_t index;
    size_t size;
} Token;

int is_keyword(char *word, size_t word_s) {
    for(size_t i = 0; i < keywords_s; i++) {
        if(word_s < strlen(keywords[i])) continue;
        if(strcmp(word, keywords[i]) == 0) return 1;
    }
    return 0;
}

int is_type(char *word, size_t word_s) {
    for(size_t i = 0; i < types_s; i++) {
        if(word_s < strlen(types[i])) continue;
        if(strcmp(word, types[i]) == 0) return 1;
    }
    return 0;
}

size_t read_file_to_str(char *filename, char **contents) {
    FILE *file = fopen(filename, "r");
    if(file == NULL) {
        endwin();
        fprintf(stderr, "could not read from file: %s\n", filename);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    *contents = malloc(sizeof(char)*length);
    fread(*contents, 1, length, file);
    fclose(file);
    contents[length] = '\0';
    return length;
}

Custom_Color *parse_syntax_file(char *filename) {
    char *contents = NULL;
    size_t contents_s = read_file_to_str(filename, &contents);
    String_View contents_view = view_create(contents, contents_s);

    size_t num_of_dots = 0;
    for(size_t i = 0; i < contents_view.len; i++) {
        if(contents_view.data[i] == '.') {
            num_of_dots++;
        }
    }

    String_View *lines = malloc(sizeof(String_View)*num_of_dots);
    size_t lines_s = 0;

    size_t cur_size = 0;
    char *cur = contents_view.data;
    for(size_t i = 0; i <= contents_view.len; i++) {
        cur_size++;
        if(contents_view.data[i-1] == '.') {
            lines[lines_s].data = cur;
            cur += cur_size;
            lines[lines_s++].len = cur_size;
            cur_size = 0;
        }
    }

    endwin();

    for(size_t i = 0; i < lines_s; i++) {
        size_t num_of_commas = 0;
        for(size_t j = 0; j < lines[i].len; j++) {
            if(lines[i].data[j] == ',') {
                num_of_commas++;
            }
        }
        String_View words[num_of_commas];
        size_t words_s = 0;
        char *cur = lines[i].data;
        size_t cur_size = 0;
        for(size_t j = 0; j < lines[i].len; j++) {
            cur_size++;
            if(lines[i].data[j] == ',') {
                words[words_s].data = cur;
                cur += cur_size;
                words[words_s++].len = cur_size-1;
                cur_size = 0;
            }
        }
        cur_size--;
        words[words_s].data = cur;
        cur += cur_size;
        words[words_s++].len = cur_size-1;

        Custom_Color color = {0};
        color.custom_id = i+8;
        char cur_type = words[0].data[0]; 
        color.custom_r = view_to_int(words[1]);
        color.custom_g = view_to_int(words[2]);
        color.custom_b = view_to_int(words[3]);
        if(cur_type == 'k') {
            keywords = malloc(sizeof(char*)*words_s-3);
            color.custom_slot = 4;
        } else if(cur_type == 't') {
            types = malloc(sizeof(char*)*words_s-3);
            color.custom_slot = 1;
        } else if(cur_type == 'w') {
            color.custom_slot = 2;
        }

        for(size_t j = 4; j < words_s; j++) {
            switch(cur_type) {
                case 'k':
                    keywords[keywords_s++] = view_to_cstr(words[j]);
                    break;
                case 't':
                    types[types_s++] = view_to_cstr(words[j]);
                    break;
                case 'w':
                    break;
                default:
                    break;
            }
        }
    }

    exit(1);
    return (Custom_Color*){0}; 
}

int is_in_tokens_index(Token *token_arr, size_t token_s, size_t index, size_t *size, Color_Pairs *color) {
    for(size_t i = 0; i < token_s; i++) {
        if(token_arr[i].index == index) {
            *size = token_arr[i].size;
            switch(token_arr[i].type) {
                case Type_None:
                    break;
                case Type_Keyword:
                    *color = RED_COLOR;
                    break;
                case Type_Type:
                    *color = YELLOW_COLOR;
                    break;
                case Type_Preprocessor:
                    *color = CYAN_COLOR;
                    break;
                case Type_String:
                    *color = MAGENTA_COLOR;
                    break;
                case Type_Comment:
                    *color = GREEN_COLOR;
                    break;
                case Type_Word:
                    *color = BLUE_COLOR;
                    break;
            }
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
        if(word_s >= 32) break;
        word[word_s++] = view->data[0]; 
        view->data++;
        view->len--;
    }
    view->data--;
    view->len++;
    if(is_keyword(word, word_s)) {
        return (Token){.type = Type_Keyword, .index = index, .size = word_s};
    } else if(is_type(word, word_s)) {
        return (Token){.type = Type_Type, .index = index, .size = word_s};
    } else {
        return (Token){.type = Type_Word, .index = index, .size = word_s};
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
        } else if(view.data[0] == '#') {
            Token token = {
                .type = Type_Preprocessor,
                .index = 0,
                .size = view.len,
            };

            while(view.len > 0 && view.data[0] != '\n') {
                view.len--;
                view.data++;
            }
            token_arr[token_arr_s++] = token;
        } else if(view.len >= 2 && view.data[0] == '/' && view.data[1] == '/') {
            Token token = {
                .type = Type_Comment,
                .index = view.data-line,
                .size = view.len,
            };
            while(view.len > 0 && view.data[0] != '\n') {
                view.len--;
                view.data++;
            }
            token_arr[token_arr_s++] = token;
        } else if(view.data[0] == '"') {
            Token token = {
                .type = Type_String,
                .index = view.data-line,
            };
            size_t string_s = 1;
            view.len--;
            view.data++;
            while(view.len > 0 && view.data[0] != '"') {
                string_s++;
                view.len--;
                view.data++;
            }
            token.size = ++string_s;
            token_arr[token_arr_s++] = token;
        }
        if(view.len == 0) break;
        view.data++;
        view.len--;
    }
    return token_arr_s;
}

int read_file_by_lines(char *filename, char **lines, size_t *lines_s) {
    FILE *file = fopen(filename, "r");
    if(file == NULL) {
        return 1;
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if(length == 0) {
        fclose(file);
        return 1;
    }

    char *contents = malloc(sizeof(char)*length);
    fread(contents, 1, length, file);
    fclose(file);

    size_t line_count = 0;
    for(size_t i = 0; i < length; i++) {
        if(contents[i] == '\n') line_count++;
    }
    free(lines);

    char **new_lines = malloc(sizeof(*lines)*line_count);

    char current_line[128] = {0};
    size_t current_line_s = 0;
    for(size_t i = 0; i < length; i++) {
        if(contents[i] == '\n') {
            new_lines[*lines_s] = malloc(sizeof(char)*current_line_s+1);
            strncpy(new_lines[*lines_s], current_line, current_line_s+1);
            current_line_s = 0;
            memset(current_line, 0, current_line_s);
            *lines_s += 1;
            continue;
        }
        current_line[current_line_s++] = contents[i];
    }

    lines = new_lines;

    free(contents);
    return 0;
}
