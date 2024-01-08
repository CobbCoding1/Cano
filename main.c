#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <assert.h>
#include <string.h>

#define ctrl(x) ((x) & 0x1f)

#define BACKSPACE 127 
#define ESCAPE    27
#define ENTER     10

typedef enum {
    NORMAL,
    INSERT,
} Mode;

#define MAX_STRING_SIZE 1025

typedef struct {
    size_t index;
    size_t size;
    char *contents;
} Row;

#define MAX_ROWS 1024
typedef struct {
    Row rows[MAX_ROWS];
    size_t row_index;
    size_t cur_pos;
    size_t row_s;
    char *filename;
} Buffer;

Mode mode = NORMAL;
int QUIT = 0;

char *stringify_mode() {
    switch(mode) {
        case NORMAL:
            return "NORMAL";
            break;
        case INSERT:
            return "INSERT";
            break;
        default:
            return "NORMAL";
            break;
    }
}

// shift_rows_* functions shift the entire array of rows
void shift_rows_left(Buffer *buf, size_t index) {
    assert(buf->row_s+1 < MAX_ROWS);
    for(size_t i = index; i < buf->row_s; i++) {
        buf->rows[i] = buf->rows[i+1];
    }
    buf->rows[buf->row_s].size = 0;
    buf->row_s--;
}

void shift_rows_right(Buffer *buf, size_t index) {
    assert(buf->row_s+1 < MAX_ROWS);
    char *new = calloc(MAX_STRING_SIZE, sizeof(char));
    for(size_t i = buf->row_s+1; i > index; i--) {
        buf->rows[i] = buf->rows[i-1];
    }
    buf->rows[index] = (Row){0};
    buf->rows[index].contents = new;
    buf->rows[index].index = index;
    buf->row_s++;
}

// shift_row_* functions shift single row
void shift_row_left(Row *row, size_t index) {
    for(size_t i = index; i < row->size; i++) {
        row->contents[i] = row->contents[i+1];
    }
    row->size--;  
}

void shift_row_right(Row *row, size_t index) {
    row->size++;  
    for(size_t i = row->size; i > index; i--) {
        row->contents[i] = row->contents[i-1];
    }
}
#define NO_CLEAR_

void append_rows(Row *a, Row *b) {
    assert(a->size + b->size < MAX_STRING_SIZE);
    for(size_t i = 0; i < b->size; i++) {
        a->contents[(i + a->size)] = b->contents[i];
    }
    a->size = a->size + b->size;
}

void delete_and_append_row(Buffer *buf, size_t index) {
    append_rows(&buf->rows[index-1], &buf->rows[index]);
    shift_rows_left(buf, index); 
}

void create_and_cut_row(Buffer *buf, size_t dest_index, size_t *str_s, size_t index) {
    assert(index < MAX_STRING_SIZE);
    assert(dest_index > 0);
    size_t final_s = *str_s - index;
    char *temp = calloc(final_s, sizeof(char));
    size_t temp_len = 0;
    for(size_t i = index; i < *str_s; i++) {
        temp[temp_len++] = buf->rows[dest_index-1].contents[i];
        buf->rows[dest_index-1].contents[i] = '\0';
    }
    shift_rows_right(buf, dest_index);
    strncpy(buf->rows[dest_index].contents, temp, sizeof(char)*final_s);
    buf->rows[dest_index].size = final_s;
    *str_s = index;
    free(temp);
}

void read_file_to_buffer(Buffer *buffer, char *filename) {
    buffer->filename = filename;
    FILE *file = fopen(filename, "a+");
    if(file == NULL) {
        endwin();
        fprintf(stderr, "error: could not open file %s\n", filename);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buf = malloc(sizeof(char)*length);
    fread(buf, sizeof(char)*length, 1, file);
    for(size_t i = 0; i+1 < length; i++) {
        if(buf[i] == '\n') {
            buffer->row_s++;
            continue;
        }
        buffer->rows[buffer->row_s].contents[buffer->rows[buffer->row_s].size++] = buf[i];
    }
}

typedef struct {
    char brace;
    int closing;
} Brace;

Brace find_opposite_brace(char opening) {
    switch(opening) {
        case '(':
            return (Brace){.brace = ')', .closing = 0};
            break;
        case '{':
            return (Brace){.brace = '}', .closing = 0};
            break;
        case '[':
            return (Brace){.brace = ']', .closing = 0};
            break;
        case ')':
            return (Brace){.brace = '(', .closing = 1};
            break;
        case '}':
            return (Brace){.brace = '{', .closing = 1};
            break;
        case ']':
            return (Brace){.brace = '[', .closing = 1};
            break;
    }
    return (Brace){.brace = '0'};
}

int main(int argc, char *argv[]) {
    char *program = argv[0];
    (void)program;
    char *filename = NULL;
    if(argc > 1) {
        filename = argv[1];
    }
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    Buffer buffer = {0};
    for(size_t i = 0; i < 1024; i++) {
        buffer.rows[i].contents = calloc(MAX_STRING_SIZE, sizeof(char));
    }
    if(filename != NULL) read_file_to_buffer(&buffer, filename);
    else buffer.filename = "out.txt";

    int row, col;
    getmaxyx(stdscr, row, col);

    mvprintw(row-1, 0, "%.6s", stringify_mode());
    move(0, 0);

    int ch = 0;

    size_t x, y = 0;
    while(ch != ctrl('q') && QUIT != 1) {
#ifndef NO_CLEAR
        clear();
#endif
        getmaxyx(stdscr, row, col);
        refresh();
        mvprintw(row-1, 0, "%.6s", stringify_mode());
        mvprintw(row-1, col/2, "%.3zu:%.3zu", buffer.row_index, buffer.cur_pos);
        
        for(size_t i = 0; i <= buffer.row_s; i++) {
            mvprintw(i, 0, "%s", buffer.rows[i].contents);
        }

        move(y, x);
        ch = getch();
        switch(mode) {
            case NORMAL:
                switch(ch) {
                    case 'i':
                        mode = INSERT;
                        keypad(stdscr, FALSE);
                        break;
                    case 'h':
                        if(buffer.cur_pos != 0) buffer.cur_pos--;
                        break;
                    case 'j':
                        if(buffer.row_index < buffer.row_s) buffer.row_index++;
                        break;
                    case 'k':
                        if(buffer.row_index != 0) buffer.row_index--;
                        break;
                    case 'l':
                        buffer.cur_pos++;
                        break;
                    case 'x': {
                        Row *cur = &buffer.rows[buffer.row_index];
                        if(cur->size > 0) {
                            shift_row_left(cur, buffer.cur_pos);
                            move(y, buffer.cur_pos);
                        }
                    } break;
                    case 'd': {
                        Row *cur = &buffer.rows[buffer.row_index];
                        memset(cur->contents, 0, cur->size);
                        cur->size = 0;
                        if(buffer.row_s != 0) {
                            if(buffer.row_index == buffer.row_s) buffer.row_index--;
                            shift_rows_left(&buffer, cur->index);
                        }
                    } break;
                    case 'g':
                        buffer.row_index = 0;
                        break;
                    case 'G':
                        buffer.row_index = buffer.row_s;
                        break;
                    case '0':
                        buffer.cur_pos = 0;
                        break;
                    case '$':
                        buffer.cur_pos = buffer.rows[buffer.row_index].size;
                        break;
                    case 'e': {
                        Row *cur = &buffer.rows[buffer.row_index];
                        if(cur->contents[buffer.cur_pos+1] == ' ') buffer.cur_pos++;
                        while(cur->contents[buffer.cur_pos+1] != ' ' && buffer.cur_pos+1 < cur->size) {
                            buffer.cur_pos++;
                        }
                    } break;
                    case 'b': {
                        Row *cur = &buffer.rows[buffer.row_index];
                        if(cur->contents[buffer.cur_pos-1] == ' ') buffer.cur_pos--;
                        while(cur->contents[buffer.cur_pos-1] != ' ' && buffer.cur_pos > 0) {
                            buffer.cur_pos--;
                        }
                    } break;
                    case 'w': {
                        Row *cur = &buffer.rows[buffer.row_index];
                        if(cur->contents[buffer.cur_pos-1] == ' ') buffer.cur_pos++;
                        while(cur->contents[buffer.cur_pos-1] != ' ' && buffer.cur_pos < cur->size) {
                            buffer.cur_pos++;
                        }
                    } break;
                    case '%': {
                        Row *cur = &buffer.rows[buffer.row_index];
                        Brace opposite = find_opposite_brace(cur->contents[buffer.cur_pos]);
                        char brace_stack[64] = {0};
                        size_t brace_stack_s = 0;
                        if(opposite.brace == '0') break;
                        if(opposite.closing) {
                            int posx = buffer.cur_pos;
                            int posy = buffer.row_index;
                            while(posy >= 0) {
                                posx--;
                                if(posx < 0) {
                                    cur = &buffer.rows[--posy];
                                    posx = cur->size;
                                }
                                Brace new_brace = find_opposite_brace(cur->contents[posx]);
                                if(new_brace.brace != '0' && new_brace.closing) brace_stack[brace_stack_s++] = new_brace.brace;
                                if(new_brace.brace != '0' && !new_brace.closing) {
                                    if(brace_stack_s == 0) break;
                                    brace_stack[--brace_stack_s] = new_brace.brace;
                                }
                            }
                            if(posx >= 0 && posy >= 0) {
                                buffer.cur_pos = posx;
                                buffer.row_index = posy;
                            }
                            break;
                        }
                        size_t initial_x = buffer.cur_pos;
                        size_t initial_y = buffer.row_index;
                        while(buffer.row_index <= buffer.row_s) {
                            buffer.cur_pos++;
                            if(buffer.cur_pos > cur->size) {
                                cur = &buffer.rows[++buffer.row_index];
                                buffer.cur_pos = 0;
                            }
                            Brace new_brace = find_opposite_brace(cur->contents[buffer.cur_pos]);
                            if(new_brace.brace != '0' && !new_brace.closing) brace_stack[brace_stack_s++] = new_brace.brace;
                            if(new_brace.brace != '0' && new_brace.closing) {
                                if(brace_stack_s == 0) break;
                                brace_stack[--brace_stack_s] = new_brace.brace;
                            }
                        }
                        if(buffer.row_index > buffer.row_s) {
                            buffer.row_index = initial_y;
                            buffer.cur_pos = initial_x;
                        }
                        break;
                    }
                    case ctrl('s'): {
                        FILE *file = fopen(buffer.filename, "w"); 
                        for(size_t i = 0; i <= buffer.row_s; i++) {
                            fwrite(buffer.rows[i].contents, buffer.rows[i].size, 1, file);
                            fwrite("\n", sizeof("\n")-1, 1, file);
                        }
                        fclose(file);
                        QUIT = 1;
                    } break;
                    default:
                        continue;
                }
                if(buffer.cur_pos > buffer.rows[buffer.row_index].size) buffer.cur_pos = buffer.rows[buffer.row_index].size;
                x = buffer.cur_pos;
                y = buffer.row_index;
                move(y, x);
                break;
            case INSERT: {
                keypad(stdscr, FALSE);
                if(ch == BACKSPACE) {
                    getyx(stdscr, y, x);
                    if(buffer.cur_pos == 0) {
                        if(buffer.row_index != 0) {
                            Row *cur = &buffer.rows[--buffer.row_index];
                            buffer.cur_pos = cur->size;
                            move(buffer.row_index, buffer.cur_pos);
                            delete_and_append_row(&buffer, cur->index+1);
                        }
                    } else {
                        Row *cur = &buffer.rows[buffer.row_index];
                        shift_row_left(cur, --buffer.cur_pos);
                        move(y, buffer.cur_pos);
                    }
                } else if(ch == ESCAPE) {
                    mode = NORMAL;
                    keypad(stdscr, TRUE);
                } else if(ch == ENTER) {
                    Row *cur = &buffer.rows[buffer.row_index]; 
                    create_and_cut_row(&buffer, buffer.row_index+1,
                                &cur->size, buffer.cur_pos);
                    buffer.row_index++; 
                    buffer.cur_pos = 0;
                    move(buffer.row_index, buffer.cur_pos);
                } else {
                    Row *cur = &buffer.rows[buffer.row_index];
                    shift_row_right(cur, buffer.cur_pos);
                    cur->contents[buffer.cur_pos++] = ch;
                    move(y, buffer.cur_pos);
                }
                break;
             }
        }
        getyx(stdscr, y, x);
    }

    refresh();
    endwin();
    return 0;
}
