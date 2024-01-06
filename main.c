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

void shift_rows(Buffer *buf, size_t index) {
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

void shift_str(Buffer *buf, size_t dest_index, size_t *dest_s, size_t *str_s, size_t index) {
    assert(index < MAX_STRING_SIZE);
    assert(dest_index > 0);
    *dest_s = (*str_s - index);
    size_t final_s = *dest_s;
    char *temp = calloc(final_s, sizeof(char));
    for(size_t i = index; i < *str_s; i++) {
        temp[i % index] = buf->rows[dest_index-1].contents[i];
        buf->rows[dest_index-1].contents[i] = '\0';
    }
    shift_rows(buf, dest_index);
    strncpy(buf->rows[dest_index].contents, temp, sizeof(char)*final_s);
    buf->rows[dest_index].size = final_s;
    *str_s = index;
    free(temp);
}

int main(void) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    Buffer buffer = {0};
    for(size_t i = 0; i < 1024; i++) {
        buffer.rows[i].contents = calloc(MAX_STRING_SIZE, sizeof(char));
    }

    int row, col;
    getmaxyx(stdscr, row, col);

    mvprintw(row-1, 0, stringify_mode());
    move(0, 0);

    int ch = 0;

    size_t x, y = 0;
    while(ch != ctrl('q') && QUIT != 1) {
        clear();
        getmaxyx(stdscr, row, col);
        refresh();
        mvprintw(row-1, 0, stringify_mode());
        mvprintw(row-1, col/2, "%.3zu:%.3zu", buffer.row_index, buffer.cur_pos);
        
        for(size_t i = 0; i <= buffer.row_s; i++) {
            mvprintw(i, 0, "%s", buffer.rows[i].contents);
        }

        move(y, x);
        ch = getch();
        switch(mode) {
            case NORMAL:
                if(ch == 'i') {
                    mode = INSERT;
                } else if(ch == 'h') {
                    if(buffer.cur_pos != 0) buffer.cur_pos--;
                } else if(ch == 'l') {
                    buffer.cur_pos++;
                } else if(ch == 'k') {
                    if(buffer.row_index != 0) buffer.row_index--;
                } else if(ch == 'j') {
                    if(buffer.row_index < buffer.row_s) buffer.row_index++;
                } else if(ch == ctrl('s')) {
                    FILE *file = fopen("out.txt", "w"); 
                    for(size_t i = 0; i <= buffer.row_s; i++ ) {
                        fwrite(buffer.rows[i].contents, buffer.rows[i].size, 1, file);
                        fwrite("\n", sizeof("\n")-1, 1, file);
                    }
                    fclose(file);
                    QUIT = 1;
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
                        }
                    } else {
                        Row *cur = &buffer.rows[buffer.row_index];
                        cur->contents[--buffer.cur_pos] = ' ';
                        cur->size = buffer.cur_pos;
                        move(y, buffer.cur_pos);
                    }
                } else if(ch == ESCAPE) {
                    mode = NORMAL;
                    keypad(stdscr, TRUE);
                } else if(ch == ENTER) {
                    Row *cur = &buffer.rows[buffer.row_index]; 
                    Row *next = &buffer.rows[buffer.row_index+1]; 
                    shift_str(&buffer, buffer.row_index+1, &next->size, 
                                &cur->size, buffer.cur_pos);
                    buffer.row_index++; 
                    buffer.cur_pos = 0;
                    move(buffer.row_index, buffer.cur_pos);
                } else {
                    Row *cur = &buffer.rows[buffer.row_index];
                    cur->contents[buffer.cur_pos++] = ch;
                    cur->size = buffer.cur_pos;
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
