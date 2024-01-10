#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <assert.h>
#include <string.h>

#define ctrl(x) ((x) & 0x1f)

#define BACKSPACE   263 
#define ESCAPE      27
#define ENTER       10
#define DOWN_ARROW  258 
#define UP_ARROW    259 
#define LEFT_ARROW  260 
#define RIGHT_ARROW 261 

typedef enum {
    NORMAL,
    INSERT,
} Mode;

int ESCDELAY = 10;

// global config vars
int relative_nums = 0;

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

typedef enum {
    LINE_NUMS = 1,
} Color_Pairs;

int main(int argc, char *argv[]) {
    char *program = argv[0];
    (void)program;
    char *filename = NULL;
    if(argc > 1) {
        filename = argv[1];
    }
    initscr();
    if(has_colors() == FALSE) {
        endwin();
        fprintf(stderr, "error: you do not have colors in your terminal\n");
        exit(1);
    }

    // colors
    start_color();
    init_pair(LINE_NUMS, COLOR_YELLOW, COLOR_BLACK);

    int grow, gcol;
    getmaxyx(stdscr, grow, gcol);
    raw();
    WINDOW *main_win = newwin(grow*0.95, gcol+5, 0, 5);
    WINDOW *line_num_win = newwin(grow*0.95, 5, 0, 0);

    int row, col;
    getmaxyx(main_win, row, col);
    (void)row; (void)col;

    WINDOW *status_bar = newwin(grow*0.05, gcol, grow-2, 0);
    wrefresh(main_win);
    wrefresh(status_bar);
    wrefresh(line_num_win);
    keypad(main_win, TRUE);
    noecho();

    Buffer buffer = {0};
    for(size_t i = 0; i < 1024; i++) {
        buffer.rows[i].contents = calloc(MAX_STRING_SIZE, sizeof(char));
    }
    if(filename != NULL) read_file_to_buffer(&buffer, filename);
    else buffer.filename = "out.txt";

    getmaxyx(main_win, row, col);
    mvwprintw(status_bar, 0, 0, "%.6s", stringify_mode());
    wmove(main_win, 0, 0);

    int ch = 0;

    size_t line_render_start = 0;

    size_t x = 0; 
    size_t y = 0;
    while(ch != ctrl('q') && QUIT != 1) {
#ifndef NO_CLEAR
        wclear(main_win);
        wclear(status_bar);
        wclear(line_num_win);
#endif
        getmaxyx(main_win, row, col);
        // status bar
        mvwprintw(status_bar, 0, 0, "%.6s", stringify_mode());
        mvwprintw(status_bar, 0, gcol/2, "%.3zu:%.3zu", buffer.row_index+1, buffer.cur_pos+1);

        if(buffer.row_index <= line_render_start) line_render_start = buffer.row_index;
        if(buffer.row_index >= line_render_start+row) line_render_start = buffer.row_index-row+1;
        
        for(size_t i = line_render_start; i <= line_render_start+row; i++) {
            size_t print_index = i - line_render_start;
            wattron(line_num_win, COLOR_PAIR(LINE_NUMS));
            if(relative_nums) {
                if(buffer.row_index == print_index) mvwprintw(line_num_win, print_index, 0, "%4zu", i+1);
                else mvwprintw(line_num_win, print_index, 0, "%4zu", (size_t)abs((int)buffer.row_index-(int)print_index));
            } else {
                mvwprintw(line_num_win, print_index, 0, "%4zu", i+1);
            }
            wattroff(line_num_win, COLOR_PAIR(LINE_NUMS));

            mvwprintw(main_win, print_index, 0, "%s", buffer.rows[i].contents);
        }

        wrefresh(main_win);
        wrefresh(status_bar);
        wrefresh(line_num_win);

        y = buffer.row_index-line_render_start;
        x = buffer.cur_pos;

        wmove(main_win, y, x);
        ch = wgetch(main_win);
        switch(mode) {
            case NORMAL:
                switch(ch) {
                    case 'i':
                        mode = INSERT;
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
                            wmove(main_win, y, buffer.cur_pos);
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
                        if(opposite.brace == '0') break;
                        size_t brace_stack_s = 0;
                        int posx = buffer.cur_pos;
                        int posy = buffer.row_index;
                        int dif = (opposite.closing) ? -1 : 1;
                        while(posy >= 0 && (size_t)posy <= buffer.row_s) {
                            posx += dif;
                            if(posx < 0 || (size_t)posx > cur->size) {
                                if(posy == 0 && dif == -1) break;
                                posy += dif;
                                cur = &buffer.rows[posy];
                                posx = (posx < 0) ? cur->size : 0;
                            }
                            opposite = find_opposite_brace(cur->contents[posx]);
                            if(opposite.brace == '0') continue; 
                            if((opposite.closing && dif == -1) || (!opposite.closing && dif == 1)) {
                                brace_stack_s++;
                            } else {
                                if(brace_stack_s-- == 0) break;
                            }
                        }
                        if((posx >= 0 && posy >= 0) && ((size_t)posy <= buffer.row_s)) {
                            buffer.cur_pos = posx;
                            buffer.row_index = posy;
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
                wmove(main_win, y, x);
                break;
            case INSERT: {
                switch(ch) {
                    case BACKSPACE: {
                        getyx(main_win, y, x);
                        if(buffer.cur_pos == 0) {
                            if(buffer.row_index != 0) {
                                Row *cur = &buffer.rows[--buffer.row_index];
                                buffer.cur_pos = cur->size;
                                wmove(main_win, buffer.row_index, buffer.cur_pos);
                                delete_and_append_row(&buffer, cur->index+1);
                            }
                        } else {
                            Row *cur = &buffer.rows[buffer.row_index];
                            shift_row_left(cur, --buffer.cur_pos);
                            wmove(main_win, y, buffer.cur_pos);
                        }
                    } break;
                    case ESCAPE:
                        mode = NORMAL;
                        break;
                    case ENTER: {
                        Row *cur = &buffer.rows[buffer.row_index]; 
                        create_and_cut_row(&buffer, buffer.row_index+1,
                                    &cur->size, buffer.cur_pos);
                        buffer.row_index++; 
                        buffer.cur_pos = 0;
                    } break;
                    case LEFT_ARROW:
                        if(buffer.cur_pos != 0) buffer.cur_pos--;
                        break;
                    case DOWN_ARROW:
                        if(buffer.row_index < buffer.row_s) buffer.row_index++;
                        break;
                    case UP_ARROW:
                        if(buffer.row_index != 0) buffer.row_index--;
                        break;
                    case RIGHT_ARROW:
                        if(buffer.cur_pos < buffer.rows[buffer.row_index].size) buffer.cur_pos++;
                        break;
                    case ctrl('s'):
                        break;
                    default: {
                        Row *cur = &buffer.rows[buffer.row_index];
                        shift_row_right(cur, buffer.cur_pos);
                        cur->contents[buffer.cur_pos++] = ch;
                        Brace next_ch = find_opposite_brace(ch); 
                        if(next_ch.brace != '0' && !next_ch.closing) {
                            shift_row_right(cur, buffer.cur_pos);
                            cur->contents[buffer.cur_pos] = next_ch.brace;
                        } 
                    } break;
                }
             }
        }
        if(buffer.cur_pos > buffer.rows[buffer.row_index].size) buffer.cur_pos = buffer.rows[buffer.row_index].size;
        x = buffer.cur_pos;
        y = buffer.row_index;
        wmove(main_win, y, x);
        getyx(main_win, y, x);
        getmaxyx(stdscr, grow, gcol);
    }

    wrefresh(main_win);
    wrefresh(status_bar);
    wrefresh(line_num_win);
    endwin();
    return 0;
}
