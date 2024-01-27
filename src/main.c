#include "main.h"

int is_between(Point a, Point b, Point c) {
    if(a.y == b.y) {
        if(c.y == a.y && c.x <= b.x && c.x >= a.x) return 1;
    } else if ((a.y <= c.y && c.y <= b.y) || (b.y <= c.y && c.y <= a.y)) {
        if(c.y == b.y && c.x > b.x) return 0;
        if(c.y == a.y && c.x < a.x) return 0;
        return 1;  
    }
    return 0; 
}

char *string_modes[MODE_COUNT] = {"NORMAL", "INSERT", "SEARCH", "COMMAND", "VISUAL"};
_Static_assert(sizeof(string_modes)/sizeof(*string_modes) == MODE_COUNT, "Number of modes");

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


Ncurses_Color rgb_to_ncurses(int r, int g, int b) {

    Ncurses_Color color = {0};

    color.r = (int) ((r / 256.0) * 1000);
    color.g = (int) ((g / 256.0) * 1000);
    color.b = (int) ((b / 256.0) * 1000);
    return color;

}


void init_ncurses_color(int id, int r, int g, int b) {
        Ncurses_Color color = rgb_to_ncurses(r, g, b);
        init_color(id, color.r, color.g, color.b);
}

void free_buffer(Buffer **buffer) {
    for(size_t i = 0; i < (*buffer)->row_capacity; i++) {
        if((*buffer)->rows[i].contents != NULL) {
            free((*buffer)->rows[i].contents);
            (*buffer)->rows[i].contents = NULL;
        }
    }
    free((*buffer)->rows);
    free((*buffer)->filename);
    free(*buffer);
    *buffer = NULL;
}


Buffer *copy_buffer(Buffer *buffer) {
    Buffer *buf = calloc(1, sizeof(Buffer));
    *buf = *buffer;
    size_t filename_s = strlen(buffer->filename)+1;
    buf->filename = calloc(filename_s, sizeof(char));
    strncpy(buf->filename, buffer->filename, filename_s);
    buf->rows = calloc(buffer->row_capacity, sizeof(Row));
    for(size_t i = 0; i < buffer->row_capacity; i++) {
        buf->rows[i].contents = calloc(buffer->rows[i].capacity, sizeof(char));
    }
    for(size_t i = 0; i <= buffer->row_s; i++) {
        Row *cur = &buffer->rows[i];
        buf->rows[i].size = cur->size; 
        buf->rows[i].capacity = cur->capacity; 
        buf->rows[i].index = cur->index; 
        strncpy(buf->rows[i].contents, cur->contents, buffer->rows[i].size);
    }
    return buf;
}


void shift_undo_left(Undo *undo, size_t amount) {
    for(size_t j = 0; j < amount; j++) {
        if(undo->buf_stack[0] != NULL) free_buffer(&undo->buf_stack[0]);
        for(size_t i = 1; i < undo->buf_stack_s; i++) {
            undo->buf_stack[i-1] = undo->buf_stack[i];
        }
        undo->buf_stack_s--;
    }
}


void push_undo(Undo *undo, Buffer *buf) {
    if(undo->buf_stack_s >= undo->buf_capacity) {
        shift_undo_left(undo, 1); 
    }
    Buffer *result = copy_buffer(buf); 
    undo->buf_stack[undo->buf_stack_s++] = result;
}


Buffer *pop_undo(Undo *undo) {
    if(undo->buf_stack_s == 0) return NULL;
    Buffer *result = copy_buffer(undo->buf_stack[--undo->buf_stack_s]);
    if(result != NULL) {
        free_buffer(&undo->buf_stack[undo->buf_stack_s]);
    }
    return result;
}


void resize_rows(Buffer *buffer, size_t capacity) {
    Row *rows = calloc(capacity*2, sizeof(Row));
    if(rows == NULL) {
        CRASH("no more ram");
    }
    memcpy(rows, buffer->rows, sizeof(Row)*buffer->row_capacity);
    buffer->rows = rows;
    for(size_t i = buffer->row_s+1; i < capacity*2; i++) {
        buffer->rows[i].capacity = STARTING_ROW_SIZE;
        buffer->rows[i].contents = calloc(buffer->rows[i].capacity, sizeof(char));
        if(buffer->rows[i].contents == NULL) {
            CRASH("no more ram");
        }
    }
    buffer->row_capacity = capacity * 2;
}

void resize_row(Row **row, size_t capacity) {
    (*row)->capacity = capacity;
    char *new_contents = realloc((*row)->contents, (*row)->capacity);
    if(new_contents == NULL) {
        CRASH("no more ram");
    }
    memset(new_contents+(*row)->size, 0, capacity-(*row)->size);
    (*row)->contents = new_contents;
    return;
}

void insert_char(Row *row, size_t pos, char c) {
    if(pos >= row->capacity) {
        resize_row(&row, row->capacity*2);
    }
    row->contents[pos] = c;
}

Point search(Buffer *buffer, char *command, size_t command_s) {
    Point point = {
            .x = buffer -> cur_pos,
            .y = buffer -> row_index,
    };
    for(size_t i = buffer->row_index; i <= buffer->row_s+buffer->row_index; i++) {

        size_t index = (i + buffer->row_s+1) % (buffer->row_s+1);
        Row *cur = &buffer->rows[index];
        size_t j = (i == buffer->row_index) ? buffer->cur_pos+1 : 0;
        for(; j < cur->size; j++) {
            if(strncmp(cur->contents+j, command, command_s) == 0) {
                // result found and will return the location of the word.
                point.x = j;
                point.y = index;
                return point;
            }
        }
    }

    return point;

}


void replace(Buffer *buffer, Point position, char *new_str, size_t old_str_s, size_t new_str_s) { 
    if (buffer == NULL || new_str == NULL) {
        WRITE_LOG("Error: null pointer");
        return;
    }

    Row *cur = &buffer->rows[position.y];
    if (cur == NULL || cur->contents == NULL) {
        WRITE_LOG("Error: null pointer");
        return;
    }

    size_t new_s = cur->size + new_str_s - old_str_s;

    for(size_t i = position.x; i < position.x+old_str_s; i++) {
        shift_row_left(cur, position.x);
    }
    cur->size = new_s;

    // Move the contents after the old substring to make space for the new string
    for(size_t i = position.x; i < position.x+new_str_s; i++) {
        shift_row_right(cur, position.x);
    }

    // Copy the new string into the buffer at the specified position
    memcpy(cur->contents + position.x, new_str, new_str_s);
}


void find_and_replace(Buffer *buffer, char *old_str, char *new_str) { 
    size_t old_str_s = strlen(old_str);
    size_t new_str_s = strlen(new_str);

    // Search for the old string in the buffer
    Point position = search(buffer, old_str, old_str_s);
    if (position.x != (buffer->cur_pos) && position.y != (buffer->row_index)){
        // If the old string is found, replace it with the new string
        replace(buffer, position, new_str, old_str_s, new_str_s);
    }
}


size_t num_of_open_braces(Buffer *buffer) {
    int posy = buffer->row_index;
    int posx = buffer->cur_pos;
    int count = 0;
    Row *cur = &buffer->rows[posy];
    while(posy >= 0) {
        posx--;
        if(posx < 0) {
            posy--;
            if(posy < 0) break;
            cur = &buffer->rows[posy];
            posx = cur->size;
        }

        Brace brace = find_opposite_brace(cur->contents[posx]);
        if(brace.brace != '0') {
            if(!brace.closing) count++; 
            if(brace.closing) count--; 
        }
    }
    if(count < 0) return 0;
    return count;
}


void reset_command(char *command, size_t *command_s) {
    assert(*command_s <= 64);
    memset(command, 0, *command_s);
    *command_s = 0;
}


void handle_save(Buffer *buffer) {
    FILE *file = fopen(buffer->filename, "w"); 
    for(size_t i = 0; i <= buffer->row_s; i++) {
        fwrite(buffer->rows[i].contents, buffer->rows[i].size, 1, file);
        fwrite("\n", sizeof("\n")-1, 1, file);
    }
    fclose(file);
}


Command parse_command(char *command, size_t command_s) {
    Command cmd = {0};
    size_t args_start = 0;
    for(size_t i = 0; i < command_s; i++) {
        if(i == command_s-1 || command[i] == ' ') {
            cmd.command_s = (i == command_s-1) ? i+1 : i;
            cmd.command = calloc(cmd.command_s+1, sizeof(char));
            strncpy(cmd.command, command, cmd.command_s);
            args_start = i+1;
            break;
        }
    }
    if(args_start <= command_s) {
        for(size_t i = args_start; i < command_s; i++) {
            if(i == command_s-1 || command[i] == ' ') {
                cmd.args[cmd.args_s].arg = calloc(i-args_start+1, sizeof(char));
                strncpy(cmd.args[cmd.args_s].arg, command+args_start, i-args_start+1);
                cmd.args[cmd.args_s++].size = i-args_start+1;
                args_start = i+1;
            }
        }
    } 
    return cmd;
}


int execute_command(Command *command, Buffer *buf, State *state) {
    if(command->command_s == 10 && strncmp(command->command, "set-output", 10) == 0) {
        if(command->args_s < 1) return INVALID_ARG; 
        char *filename = command->args[0].arg;
        free(buf->filename);
        buf->filename = calloc(command->args[0].size, sizeof(char));
        strncpy(buf->filename, filename, command->args[0].size);
    } else if(command->command_s == 1 && strncmp(command->command, "e", 1) == 0) {
        QUIT = 1;
    } else if(command->command_s == 2 && strncmp(command->command, "we", 2) == 0) {
        handle_save(buf);
        QUIT = 1;
    } else if(command->command_s == 1 && strncmp(command->command, "w", 1) == 0) {
        handle_save(buf);
    } else if(command->command_s == 7 && strncmp(command->command, "set-var", 7) == 0) {
        if(command->args_s != 2) return INVALID_ARG;
        if(command->args[0].size >= 8 && strncmp(command->args[0].arg, "relative", 8) == 0) {
            if(command->args[1].size < 1) return INVALID_VALUE;
            int value = 0;
            if(command->args[1].arg != NULL) value = atoi(command->args[1].arg);
            if(value != 0 && value != 1) return INVALID_VALUE;
            relative_nums = value;
        } else if(command->args[0].size >= 11 && strncmp(command->args[0].arg, "auto-indent", 11) == 0) {
            if(command->args[1].size < 1) return INVALID_VALUE;
            int value = 0;
            if(command->args[1].arg != NULL) value = atoi(command->args[1].arg);
            if(value != 0 && value != 1) return INVALID_VALUE;
            auto_indent = value;
        } else if(command->args[0].size >= 6 && strncmp(command->args[0].arg, "indent", 6) == 0) {
            if(command->args[1].size < 1) return INVALID_VALUE;
            int value = 0;
            if(command->args[1].arg != NULL) value = atoi(command->args[1].arg);
            if(value < 0) return INVALID_VALUE;
            indent = value;
        } else if(command->args[0].size >= 6 && strncmp(command->args[0].arg, "syntax", 6) == 0) {
            if(command->args[1].size < 1) return INVALID_VALUE;
            int value = 0;
            if(command->args[1].arg != NULL) value = atoi(command->args[1].arg);
            if(value < 0) return INVALID_VALUE;
            syntax = value;
        } else if(command->args[0].size >= 9 && strncmp(command->args[0].arg, "undo-size", 9) == 0) {
            if(command->args[1].size < 1) return INVALID_VALUE;
            int value = 0;
            if(command->args[1].arg != NULL && isdigit(command->args[1].arg[0])) value = atoi(command->args[1].arg);
            if(value < 0) return INVALID_VALUE;
            undo_size = value;
            state->undo_stack.buf_capacity = undo_size;
            for(size_t i = 0; i < state->undo_stack.buf_stack_s; i++) {
                free_buffer(&state->undo_stack.buf_stack[i]);
            }
            state->undo_stack.buf_stack_s = 0;
            free(state->undo_stack.buf_stack);
            state->undo_stack.buf_stack = calloc(state->undo_stack.buf_capacity, sizeof(Buffer));
            push_undo(&state->undo_stack, buf);
        } else {
            return UNKNOWN_COMMAND;
        }
    } else {
        return UNKNOWN_COMMAND;
    }
    for(size_t i = 0; i < command->args_s; i++) free(command->args[i].arg);
    free(command->command);
    return NO_ERROR;
}

// shift_rows_* functions shift the entire array of rows
void shift_rows_left(Buffer *buf, size_t index) {
    for(size_t i = index; i < buf->row_s; i++) {
        Row *cur = &buf->rows[i];
        Row *next = &buf->rows[i+1];
        if(next->size >= cur->capacity) resize_row(&cur, next->capacity);
        memset(cur->contents, 0, cur->size);
        strncpy(cur->contents, next->contents, next->size);
        cur->size = next->size;
        cur->capacity = next->capacity;
    }
    memset(buf->rows[buf->row_s].contents, 0, buf->rows[buf->row_s].capacity);
    buf->rows[buf->row_s].size = 0;
    buf->row_s--;
}

void shift_rows_right(Buffer *buf, size_t index) {
    if(buf->row_s+1 >= buf->row_capacity) resize_rows(buf, buf->row_capacity);
    char *new = calloc(STARTING_ROW_SIZE, sizeof(char));
    if(new == NULL) {
        CRASH("no more ram");
    }
    for(size_t i = buf->row_s+1; i > index; i--) {
        buf->rows[i] = buf->rows[i-1];
    }
    buf->rows[index] = (Row){0};
    buf->rows[index].contents = new;
    buf->rows[index].index = index;
    buf->rows[index].capacity = STARTING_ROW_SIZE;
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
    for(size_t i = row->size++; i > index; i--) {
        row->contents[i] = row->contents[i-1];
    }
    row->contents[index] = ' ';
}

void delete_char(Buffer *buffer, size_t row, size_t col, size_t *y, WINDOW *main_win) {
    Row *cur = &buffer->rows[row];
    if(cur->size > 0 && col < cur->size) {
        insert_char(cur, cur->size, '\0');
        shift_row_left(cur, col);
        wmove(main_win, *y, col);
    }
}

void delete_row(Buffer *buffer, size_t row) {
    Row *cur = &buffer->rows[row];
    memset(cur->contents, 0, cur->size);
    cur->size = 0;
    if(buffer->row_s != 0) {
        shift_rows_left(buffer, row);
        if(row > buffer->row_s) row--;
    }
}

void shift_str_left(char *str, size_t *str_s, size_t index) {
    for(size_t i = index; i < *str_s; i++) {
        str[i] = str[i+1];
    }
    *str_s -= 1;
}

void shift_str_right(char *str, size_t *str_s, size_t index) {
    *str_s += 1;
    for(size_t i = *str_s; i > index; i--) {
        str[i] = str[i-1];
    }
}

#define NO_CLEAR_

void append_rows(Row *a, Row *b) {
    if(a->size + b->size >= a->capacity) {
        resize_row(&a, a->capacity+b->capacity);
    }

    for(size_t i = 0; i < b->size; i++) {
        a->contents[(i + a->size)] = b->contents[i];
    }
    a->size = a->size + b->size;
}

void delete_and_append_row(Buffer *buf, size_t index) {
    append_rows(&buf->rows[index-1], &buf->rows[index]);
    delete_row(buf, index);
}

void create_and_cut_row(Buffer *buf, size_t dest_index, size_t *str_s, size_t index) {
    assert(index < buf->rows[buf->row_index].capacity);
    assert(dest_index > 0);

    WRITE_LOG("test1");

    size_t final_s = *str_s - index;
    char *temp = calloc(final_s, sizeof(char));
    if(temp == NULL) {
        CRASH("no more ram");
    }
    WRITE_LOG("test2");
    size_t temp_len = 0;
    for(size_t i = index; i < *str_s; i++) {
        temp[temp_len++] = buf->rows[dest_index-1].contents[i];
        buf->rows[dest_index-1].contents[i] = '\0';
    }
    shift_rows_right(buf, dest_index);
    WRITE_LOG("test3");
    Row *cur = &buf->rows[dest_index];
    if(cur->capacity < final_s) resize_row(&cur, final_s*2);
    strncpy(cur->contents, temp, sizeof(char)*final_s);
    buf->rows[dest_index].size = final_s;
    *str_s = index;
    free(temp);
    WRITE_LOG("test4");
}

void create_newline_indent(Buffer *buffer, size_t num_of_braces) {
    if(!auto_indent) return;
    Row *cur = &buffer->rows[buffer->row_index];
    Brace brace = find_opposite_brace(cur->contents[buffer->cur_pos]);
    if(brace.brace != '0' && brace.closing) {
        create_and_cut_row(buffer, buffer->row_index+1,
                    &cur->size, buffer->cur_pos);
        for(size_t i = 0; i < indent*(num_of_braces-1); i++) {
            shift_row_right(&buffer->rows[buffer->row_index+1], i);
        }
    }
    for(size_t i = 0; i < indent*num_of_braces; i++) {
        shift_row_right(cur, buffer->cur_pos);
        insert_char(cur, buffer->cur_pos++, ' ');
    }
}


Buffer *read_file_to_buffer(char *filename) {
    Buffer *buffer = calloc(1, sizeof(Buffer));
    buffer->row_capacity = STARTING_ROWS_SIZE;
    buffer->rows = calloc(buffer->row_capacity, sizeof(Row));
    for(size_t i = 0; i < buffer->row_capacity; i++) {
        buffer->rows[i].capacity = STARTING_ROW_SIZE;
        buffer->rows[i].contents = calloc(buffer->rows[i].capacity, sizeof(char));
        if(buffer->rows[i].contents == NULL) {
            CRASH("OUT OF RAM");
        }
    }
    size_t filename_s = strlen(filename)+1;
    buffer->filename = calloc(filename_s, sizeof(char));
    strncpy(buffer->filename, filename, filename_s);
    FILE *file = fopen(filename, "a+");
    if(file == NULL) {
        CRASH("could not open file");
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buf = calloc(length, sizeof(char));
    fread(buf, sizeof(char)*length, 1, file);
    if(length > buffer->row_capacity) {
        resize_rows(buffer, length);
    }

    Row *cur = &buffer->rows[buffer->row_s];
    for(size_t i = 0; i+1 < length; i++) {
        if(buf[i] == '\n') {
            buffer->row_s++;
            cur = &buffer->rows[buffer->row_s];
            continue;
        }
        cur->contents[cur->size++] = buf[i];
        if(cur->size >= cur->capacity) {
            resize_row(&cur, cur->capacity*2);
        }
    }
    return buffer;
}

int handle_motion_keys(Buffer *buffer, int ch, size_t *repeating_count) {
    switch(ch) {
        case 'g': // Move to the start of the file or to the line specified by repeating_count
            if(*repeating_count-1 > 1 && *repeating_count-1 <= buffer->row_s) {
                buffer->row_index = *repeating_count-1;
                *repeating_count = 1;
            } else buffer->row_index = 0;
            break;
        case 'G': // Move to the end of the file or to the line specified by repeating_count
            if(*repeating_count-1 > 1 && *repeating_count-1 <= buffer->row_s) {
                buffer->row_index = *repeating_count-1;
                *repeating_count = 1;
            } else buffer->row_index = buffer->row_s;
            break;
        case '0': // Move to the start of the line
            buffer->cur_pos = 0;
            break;
        case '$': // Move to the end of the line
            buffer->cur_pos = buffer->rows[buffer->row_index].size;
            break;
        case 'e': { // Move to the end of the next word
            if(buffer->row_index < buffer->row_s && buffer->cur_pos >= buffer->rows[buffer->row_index].size) {
                buffer->row_index++;
                buffer->cur_pos = 0;
            }
            Row *cur = &buffer->rows[buffer->row_index];
            if(isalnum(cur->contents[buffer->cur_pos+1])) {
                while(buffer->cur_pos+1 < cur->size && isalnum(cur->contents[buffer->cur_pos+1])) {
                    buffer->cur_pos++;
                }
            } else {
                while(buffer->cur_pos < cur->size && !isalnum(cur->contents[buffer->cur_pos+1])) {
                    buffer->cur_pos++;
                }
                while(buffer->cur_pos+1 < cur->size && isalnum(cur->contents[buffer->cur_pos+1])) {
                    buffer->cur_pos++;
                }
            }
        } break;
        case 'b': { // Move to the start of the previous word
            if(buffer->row_index > 0 && buffer->cur_pos <= 0) {
                buffer->row_index--;
                buffer->cur_pos = buffer->rows[buffer->row_index].size;
            }
            Row *cur = &buffer->rows[buffer->row_index];
            if(buffer->cur_pos > 0 && !isalnum(cur->contents[buffer->cur_pos-1])) buffer->cur_pos--;
            while(buffer->cur_pos > 0 && isalnum(cur->contents[buffer->cur_pos-1])) {
                buffer->cur_pos--;
            }
        } break;
        case 'w': { // Move to the start of the next word
            if(buffer->row_index < buffer->row_s && buffer->cur_pos >= buffer->rows[buffer->row_index].size) {
                buffer->row_index++;
                buffer->cur_pos = 0;
            }
            Row *cur = &buffer->rows[buffer->row_index];
            if(isalnum(cur->contents[buffer->cur_pos])) {
                while(buffer->cur_pos+1 < cur->size && isalnum(cur->contents[buffer->cur_pos])) {
                    buffer->cur_pos++;
                }
                buffer->cur_pos++;
            } else {
                while(buffer->cur_pos < cur->size && !isalnum(cur->contents[buffer->cur_pos])) {
                    buffer->cur_pos++;
                }
            }
        } break;
        case LEFT_ARROW:
        case 'h': // Move left
            if(buffer->cur_pos != 0) buffer->cur_pos--;
            break;
        case DOWN_ARROW:
        case 'j': // Move down
            if(buffer->row_index < buffer->row_s) buffer->row_index++;
            break;
        case UP_ARROW:
        case 'k': // Move up
            if(buffer->row_index != 0) buffer->row_index--;
            break;
        case RIGHT_ARROW:
        case 'l': // Move right
            buffer->cur_pos++;
            break;
        case '%': { // Move to the matching brace
            Row *cur = &buffer->rows[buffer->row_index];
            char initial_brace = cur->contents[buffer->cur_pos];
            Brace initial_opposite = find_opposite_brace(initial_brace);
            if(initial_opposite.brace == '0') break;
            size_t brace_stack_s = 0;
            int posx = buffer->cur_pos;
            int posy = buffer->row_index;
            // decide which direction to go (forwards or backwards) based on whether closing brace or not
            int dif = (initial_opposite.closing) ? -1 : 1;
            Brace opposite = {0};
            while(posy >= 0 && (size_t)posy <= buffer->row_s) {
                posx += dif;
                if(posx < 0 || (size_t)posx > cur->size) {
                    if(posy == 0 && dif == -1) break;
                    posy += dif;
                    cur = &buffer->rows[posy];
                    posx = (posx < 0) ? cur->size : 0;
                }
                opposite = find_opposite_brace(cur->contents[posx]);
                if(opposite.brace == '0') continue; 
                // if the brace is the same as the current then append to the stack
                // otherwise pop from it if it's opposite
                if((opposite.closing && dif == -1) || (!opposite.closing && dif == 1)) {
                    brace_stack_s++;
                } else {
                    if(brace_stack_s-- == 0 && opposite.brace == initial_brace) break;
                }
            }
            if((posx >= 0 && posy >= 0) && ((size_t)posy <= buffer->row_s)) {
                buffer->cur_pos = posx;
                buffer->row_index = posy;
            }
            break;
        }
        default: { // If the key is not a motion key, return 0
            return 0;
        } 
    }
    return 1; // If the key is a motion key, return 1
}
    
int handle_leader_keys(State *state) {
    switch(state->ch) {
        case 'd':
            state->leader = LEADER_D;
            break;
        default:
            return 0;
    }    
    return 1;
}

int handle_modifying_keys(Buffer *buffer, State *state, int ch, WINDOW *main_win, size_t *y) {
    switch(ch) {
        case 'x': {
            delete_char(buffer, buffer->row_index, buffer->cur_pos, y, main_win);
        } break;
        case 'w': {
            Row *cur = &buffer->rows[buffer->row_index];
            switch(state->leader) {
                case LEADER_D:
                    while(buffer->cur_pos < cur->size && !(!isalnum(cur->contents[buffer->cur_pos]) && isalnum(cur->contents[buffer->cur_pos+1]))) {
                        delete_char(buffer, buffer->row_index, buffer->cur_pos, &state->y, state->main_win);
                    }
                    if(isspace(cur->contents[buffer->cur_pos])) {
                        delete_char(buffer, buffer->row_index, buffer->cur_pos, &state->y, state->main_win);
                    }
                    break;
                default:
                    break;
            }
        } break;
        case 'd': {
            switch(state->leader) {
                case LEADER_D:
                    delete_row(buffer, buffer->row_index);
                    if(buffer->row_index != 0) buffer->row_index--;
                    break;
                default:
                    break;
            }
        } break;
        case 'r': {
            curs_set(0);
            ch = wgetch(main_win);
            buffer->rows[buffer->row_index].contents[buffer->cur_pos] = ch;
            curs_set(1);
        } break;
        default: {
            return 0;
        }
    }
    return 1;
}

int handle_normal_to_insert_keys(Buffer *buffer, State *state, int ch) {
    switch(ch) {
        case 'i':
            break;
        case 'I': {
            Row *cur = &buffer->rows[buffer->row_index];
            buffer->cur_pos = 0;
            while(buffer->cur_pos < cur->size && cur->contents[buffer->cur_pos] == ' ') buffer->cur_pos++;
        } break;
        case 'a':
            if(buffer->cur_pos < buffer->rows[buffer->row_index].size) buffer->cur_pos++;
            break;
        case 'A':
            buffer->cur_pos = buffer->rows[buffer->row_index].size;
            break;
        case 'o': {
            shift_rows_right(buffer, buffer->row_index+1);
            buffer->row_index++; 
            buffer->cur_pos = 0;
            if(state->num_of_braces > 0) {
                create_newline_indent(buffer, state->num_of_braces);
            }
        } break;
        case 'O': {
            shift_rows_right(buffer, buffer->row_index);
            buffer->cur_pos = 0;
            if(state->num_of_braces > 0) {
                create_newline_indent(buffer, state->num_of_braces);
            }
        } break;
        default: {
            return 0;
        }
    }
    return 1;
}

void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    if(state->leader == LEADER_NONE && handle_leader_keys(state)) {
        return;   
    } 
    switch(state->ch) {
        case ':':
            mode = COMMAND;
            buffer->cur_pos = 0;
            state->repeating.repeating_count = 1;
            break;
        case '/':
            mode = SEARCH;
            state->normal_pos = buffer->cur_pos;
            buffer->cur_pos = 0;
            reset_command(state->command, &state->command_s);
            state->repeating.repeating_count = 1;
            break;
        case 'v':
            mode = VISUAL;
            buffer->visual.is_line = 0;
            buffer->visual.starting_pos.x = buffer->cur_pos;
            buffer->visual.starting_pos.y = buffer->row_index;
            buffer->visual.ending_pos.x = buffer->cur_pos;
            buffer->visual.ending_pos.y = buffer->row_index;
            break;
        case 'V':
            mode = VISUAL;
            buffer->visual.is_line = 1;
            buffer->visual.starting_pos.x = 0;
            buffer->visual.starting_pos.y = buffer->row_index;
            buffer->visual.ending_pos.x = buffer->rows[buffer->row_index].size;
            buffer->visual.ending_pos.y = buffer->row_index;
            break;
        case ctrl('o'): {
            shift_rows_right(buffer, ++buffer->row_index);
            buffer->cur_pos = 0;
            if(auto_indent) {
                if(state->num_of_braces > 0) {
                    create_newline_indent(buffer, state->num_of_braces);
                }
            }
        } break;
        case 'R':
            state->repeating.repeating = 1;
            break;
        case 'n': {
            if(state->command_s > 2 && strncmp(state->command, "s/", 2) == 0) {
                // search and replace

                char str[128]; // replace with the maximum length of your command
                strncpy(str, state->command+2, state->command_s-2);
                str[state->command_s-2] = '\0'; // ensure null termination

                char *token = strtok(str, "/");
                int count = 0;
                char args[2][100];

                while (token != NULL) {
                    char temp_buffer[100];
                    strcpy(temp_buffer, token);
                    if(count == 0) {
                        strcpy(args[0], temp_buffer);
                    } else if(count == 1) {
                        strcpy(args[1], temp_buffer);
                    }
                    ++count;

                    // log for args.
                    token = strtok(NULL, "/");
                }
                Point new_pos = search(buffer, args[0], strlen(args[0]));
                find_and_replace(buffer, args[0], args[1]);
                buffer->cur_pos = new_pos.x;
                buffer->row_index = new_pos.y;
                break;

            } 
            Point new_pos = search(buffer, state->command, state->command_s);
            buffer->cur_pos = new_pos.x;
            buffer->row_index = new_pos.y;
        } break;
        case 'u': {
            Buffer *new_buf = pop_undo(&state->undo_stack);
            WRITE_LOG("fourth");
            if(new_buf == NULL) break;
            WRITE_LOG("fifth");
            push_undo(&state->redo_stack, buffer);
            WRITE_LOG("sixth");
            free_buffer(&buffer);
            WRITE_LOG("seventh");
            *modify_buffer = new_buf;
            WRITE_LOG("eighth");
        } break;
        case 'U': {
            Buffer *new_buf = pop_undo(&state->redo_stack);
            if(new_buf == NULL) break; 
            push_undo(&state->undo_stack, buffer);
            free_buffer(&buffer);
            *modify_buffer = new_buf;
        } break;
        case ctrl('s'): {
            handle_save(buffer);
            QUIT = 1;
            state->repeating.repeating_count = 1;
        } break;
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case KEY_RESIZE: {
            getmaxyx(stdscr, state->grow, state->gcol);
            wresize(state->main_win, state->grow-2, state->gcol-state->line_num_col);
            wresize(state->status_bar, 2, state->gcol);
            getmaxyx(state->main_win, state->main_row, state->main_col);
            mvwin(state->main_win, 0, state->line_num_col);
            wrefresh(state->main_win);
        } break;
        default: {
            int motion = (state->leader == LEADER_NONE) ? handle_motion_keys(buffer, state->ch, &state->repeating.repeating_count) : 0;
            if(motion) break;
            push_undo(&state->undo_stack, buffer);
            int modified = handle_modifying_keys(buffer, state, state->ch, state->main_win, &state->y);
            (void)modified;
            int switched = handle_normal_to_insert_keys(buffer, state, state->ch);
            if(switched) {
                mode = INSERT;
                state->repeating.repeating_count = 1;
            }
        } break;
    }
    state->leader = LEADER_NONE;
}

void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    switch(state->ch) { 
        case '\b':
        case 127:
        case KEY_BACKSPACE: { 
            if(buffer->cur_pos == 0) {
                if(buffer->row_index != 0) {
                    // Move to previous row and delete current row
                    Row *cur = &buffer->rows[--buffer->row_index];
                    buffer->cur_pos = cur->size;
                    wmove(state->main_win, buffer->row_index, buffer->cur_pos);
                    delete_and_append_row(buffer, buffer->row_index+1);
                }
            } else {
                Row *cur = &buffer->rows[buffer->row_index];
                shift_row_left(cur, --buffer->cur_pos);
                wmove(state->main_win, state->y, buffer->cur_pos);
            }
        } break;
        case ESCAPE: // Switch to NORMAL mode
            mode = NORMAL;
            break;
        case KEY_ENTER:
        case ENTER: { 
            WRITE_LOG("1");
            Row *cur = &buffer->rows[buffer->row_index]; 
            WRITE_LOG("2");
            create_and_cut_row(buffer, buffer->row_index+1, &cur->size, buffer->cur_pos);
            WRITE_LOG("3");
            buffer->row_index++; 
            buffer->cur_pos = 0;
            if(state->num_of_braces > 0) {
                create_newline_indent(buffer, state->num_of_braces);
            }
        } break;
        case LEFT_ARROW: // Move cursor left
        case DOWN_ARROW: // Move cursor down
        case UP_ARROW:   // Move cursor up
        case RIGHT_ARROW: // Move cursor right
        case KEY_RESIZE: {
            getmaxyx(stdscr, state->grow, state->gcol);
            wresize(state->main_win, state->grow-2, state->gcol-state->line_num_col);
            wresize(state->status_bar, 2, state->gcol);
            getmaxyx(state->main_win, state->main_row, state->main_col);
            mvwin(state->main_win, 0, state->line_num_col);
            wrefresh(state->main_win);
            wrefresh(state->status_bar);
        } break;
        default: { // Handle other characters
            Row *cur = &buffer->rows[buffer->row_index];
            // Handle special characters and indentation
            Brace cur_brace = find_opposite_brace(cur->contents[buffer->cur_pos]);
            if(
                (cur_brace.brace != '0' && cur_brace.closing && 
                 state->ch == find_opposite_brace(cur_brace.brace).brace) || 
                (cur->contents[buffer->cur_pos] == '"' && state->ch == '"') ||
                (cur->contents[buffer->cur_pos] == '\'' && state->ch == '\'')
            ) {
                buffer->cur_pos++;
                break;
            };
            if(state->ch == 9) {
                // TODO: use tabs instead of just 4 spaces
                for(size_t i = 0; i < indent; i++) {
                    shift_row_right(cur, buffer->cur_pos);
                    insert_char(cur, buffer->cur_pos++, ' ');
                }
            } else {
                shift_row_right(cur, buffer->cur_pos);
                insert_char(cur, buffer->cur_pos++, state->ch);
            }
            Brace next_ch = find_opposite_brace(state->ch); 
            if(next_ch.brace != '0' && !next_ch.closing) {
                shift_row_right(cur, buffer->cur_pos);
                insert_char(cur, buffer->cur_pos, next_ch.brace);
            } 
            if(state->ch == '"' || state->ch == '\'') {
                shift_row_right(cur, buffer->cur_pos);
                insert_char(cur, buffer->cur_pos, state->ch);
            }
        } break;
    }
}

void handle_command_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    switch(state->ch) {
        case '\b':
        case 127:
        case KEY_BACKSPACE: {
            if(buffer->cur_pos != 0) {
                shift_str_left(state->command, &state->command_s, --buffer->cur_pos);
                wmove(state->status_bar, 1, buffer->cur_pos);
            }
        } break;
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case KEY_ENTER:
        case ENTER: {
            if(state->command[0] == '!') {
                shift_str_left(state->command, &state->command_s, 0);
                FILE *file = popen(state->command, "r");
                if(file == NULL) {
                    CRASH("could not run command");
                }
                while(fgets(state->status_bar_msg, sizeof(state->status_bar_msg), file) != NULL) {
                    state->is_print_msg = 1;
                }
                pclose(file);
            } else {
                Command cmd = parse_command(state->command, state->command_s);
                int err = execute_command(&cmd, buffer, state);
                switch(err) {
                    case NO_ERROR:
                        break;
                    case UNKNOWN_COMMAND:
                        sprintf(state->status_bar_msg, "Unnown command: %s", cmd.command);
                        state->is_print_msg = 1;
                        break;
                    case INVALID_ARG:
                        sprintf(state->status_bar_msg, "Invalid arg %s\n", cmd.args[0].arg);
                        state->is_print_msg = 1;
                        break;
                    case INVALID_VALUE:
                        sprintf(state->status_bar_msg, "Invalid value %s\n", cmd.args[1].arg);
                        state->is_print_msg = 1;
                        break;
                    default:
                        sprintf(state->status_bar_msg, "err");
                        state->is_print_msg = 1;
                        break;
                }
            }
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
        } break;
        case LEFT_ARROW:
            if(buffer->cur_pos != 0) buffer->cur_pos--;
            break;
        case DOWN_ARROW:
            break;
        case UP_ARROW:
            break;
        case RIGHT_ARROW:
            if(buffer->cur_pos < state->command_s) buffer->cur_pos++;
            break;
        default: {
            shift_str_right(state->command, &state->command_s, buffer->cur_pos);
            state->command[buffer->cur_pos++] = state->ch;
        } break;
    }
}

void handle_search_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    switch(state->ch) {
        case '\b':
        case 127:
        case KEY_BACKSPACE: {
            if(buffer->cur_pos != 0) {
                shift_str_left(state->command, &state->command_s, --buffer->cur_pos);
                wmove(state->status_bar, 1, buffer->cur_pos);
            }
        } break;
        case ESCAPE:
            buffer->cur_pos = state->normal_pos;
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case ENTER: {
            if(state->command_s > 2 && strncmp(state->command, "s/", 2) == 0) {
                init_color(8, 250, 134, 107);

                char str[128]; // replace with the maximum length of your command
                strncpy(str, state->command+2, state->command_s-2);
                str[state->command_s-2] = '\0'; // ensure null termination

                char *token = strtok(str, "/");
                int count = 0;
                char args[2][100];

                while (token != NULL) {
                    char temp_buffer[100];
                    strcpy(temp_buffer, token);
                    if(count == 0) {
                        strcpy(args[0], temp_buffer);
                    } else if(count == 1) {
                        strcpy(args[1], temp_buffer);
                    }
                    ++count;

                    // log for args.
                    token = strtok(NULL, "/");
                }
                Point new_pos = search(buffer, args[0], strlen(args[0]));
                find_and_replace(buffer, args[0], args[1]);
                buffer->cur_pos = new_pos.x;
                buffer->row_index = new_pos.y;
                mode = NORMAL;
                break;

            } 
            Point new_pos = search(buffer, state->command, state->command_s);
            buffer->cur_pos = new_pos.x;
            buffer->row_index = new_pos.y;
            mode = NORMAL;
        } break;
        case LEFT_ARROW:
            if(buffer->cur_pos != 0) buffer->cur_pos--;
            break;
        case DOWN_ARROW:
            break;
        case UP_ARROW:
            break;
        case RIGHT_ARROW:
            if(buffer->cur_pos < state->command_s) buffer->cur_pos++;
            break;
        default: {
            shift_str_right(state->command, &state->command_s, buffer->cur_pos);
            state->command[buffer->cur_pos++] = state->ch;
        } break;
    }
}

void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    curs_set(0);
    switch(state->ch) {
        case '\b':
        case 127:
        case KEY_BACKSPACE: {
        } break;
        case ESCAPE:
            curs_set(1);
            mode = NORMAL;
            break;
        case ENTER: {
        } break;
        case 'd':
        case 'x':
            push_undo(&state->undo_stack, buffer);
            if(buffer->visual.starting_pos.y > buffer->visual.ending_pos.y || 
              (buffer->visual.starting_pos.y == buffer->visual.ending_pos.y &&
               buffer->visual.starting_pos.x > buffer->visual.ending_pos.x)) {
                for(int i = buffer->visual.starting_pos.y; i >= (int)buffer->visual.ending_pos.y; i--) {
                    Row *cur = &buffer->rows[i];
                    if(i > (int)buffer->visual.ending_pos.y && i < (int)buffer->visual.starting_pos.y) delete_row(buffer, i);
                    else {
                        for(int j = cur->size-1; j >= 0; j--) {
                            Point point = {.x = j, .y = i};
                            if(is_between(buffer->visual.ending_pos, buffer->visual.starting_pos, point)) {
                                delete_char(buffer, i, j, &state->y, state->main_win);
                            }
                        }
                    }
                }
            } else {
                for(int i = buffer->visual.ending_pos.y; i >= (int)buffer->visual.starting_pos.y; i--) {
                    Row *cur = &buffer->rows[i];
                    if(i > (int)buffer->visual.starting_pos.y && i < (int)buffer->visual.ending_pos.y) delete_row(buffer, i);
                    else {
                        for(int j = cur->size-1; j >= 0; j--) {
                            Point point = {.x = j, .y = i};
                            if(is_between(buffer->visual.starting_pos, buffer->visual.ending_pos, point)) {
                                delete_char(buffer, i, j, &state->y, state->main_win);
                            }
                        }
                    }
                }
            }
            if(buffer->visual.starting_pos.y < buffer->visual.ending_pos.y) {
                buffer->row_index = buffer->visual.starting_pos.y;
                buffer->cur_pos = buffer->visual.starting_pos.x;
            } else {
                buffer->row_index = buffer->visual.ending_pos.y;
                buffer->cur_pos = buffer->visual.ending_pos.x;
            }
            mode = NORMAL;
            curs_set(1);
            break;
        default: {
            handle_motion_keys(buffer, state->ch, &state->repeating.repeating_count);
            if(buffer->visual.is_line) {
                size_t cur_size = buffer->rows[buffer->row_index].size;
                if(buffer->row_index <= buffer->visual.starting_pos.y) {
                    buffer->visual.ending_pos.x = 0;
                    buffer->visual.starting_pos.x = cur_size;
                    buffer->visual.ending_pos.y = buffer->row_index;
                } else {
                    buffer->visual.ending_pos.x = cur_size;
                    buffer->visual.ending_pos.y = buffer->row_index;
                }
                break;
            }
            buffer->visual.ending_pos.x = buffer->cur_pos;
            buffer->visual.ending_pos.y = buffer->row_index;
        } break;
    }
}
    
State init_state() {
    State state = {0};
    state.undo_stack.buf_capacity = undo_size;
    state.redo_stack.buf_capacity = undo_size;
    state.undo_stack.buf_stack = calloc(state.undo_stack.buf_capacity, sizeof(Buffer));
    state.redo_stack.buf_stack = calloc(state.undo_stack.buf_capacity, sizeof(Buffer));
    return state;
}

void load_config_from_file(State *state, Buffer *buffer, char *config_filename, char *syntax_filename) {
    if(config_filename == NULL) {
        char default_config_filename[128] = {0};
        char config_dir[64] = {0};
        char *env = getenv("HOME");
        if(env == NULL) CRASH("could not get HOME");
        sprintf(config_dir, "%s/.config/cano", env);
        struct stat st = {0};
        if(stat(config_dir, &st) == -1) {
            mkdir(config_dir, 0755);
        }
        sprintf(default_config_filename, "%s/config.cano", config_dir);
        config_filename = default_config_filename;


        char *language = strip_off_dot(buffer->filename, strlen(buffer->filename));
        if(language != NULL) {
            syntax_filename = calloc(strlen(config_dir)+strlen(language)+sizeof(".cyntax")+1, sizeof(char));
            sprintf(syntax_filename, "%s/%s.cyntax", config_dir, language);
            free(language);
        }
    }
    char **lines = calloc(2, sizeof(char*));
    size_t lines_s = 0;
    int err = read_file_by_lines(config_filename, &lines, &lines_s);
    if(err == 0) {
        for(size_t i = 0; i < lines_s; i++) {
            Command cmd = parse_command(lines[i], strlen(lines[i]));
            execute_command(&cmd, buffer, state);
            free(lines[i]);
        }
    }
    free(lines);

    if(syntax_filename != NULL) {
        Color_Arr color_arr = parse_syntax_file(syntax_filename);
        if(color_arr.arr != NULL) {
            for(size_t i = 0; i < color_arr.arr_s; i++) {
                init_pair(color_arr.arr[i].custom_slot, color_arr.arr[i].custom_id, COLOR_BLACK);
                init_ncurses_color(color_arr.arr[i].custom_id, color_arr.arr[i].custom_r, 
                                   color_arr.arr[i].custom_g, color_arr.arr[i].custom_b);
            }

            free(color_arr.arr);
        }
    }
}

void init_colors() {
    if(has_colors() == FALSE) {
        CRASH("your terminal does not support colors");
    }

    start_color();
    init_pair(YELLOW_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(BLUE_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair(GREEN_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(RED_COLOR, COLOR_RED, COLOR_BLACK);
    init_pair(MAGENTA_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CYAN_COLOR, COLOR_CYAN, COLOR_BLACK);

}

/* ------------------------- FUNCTIONS END ------------------------- */


int main(int argc, char **argv) {

    WRITE_LOG("starting (int main)");

    (void)argc;
    char *program = *argv++;
    char *flag = *argv++;
    char *config_filename = NULL;
    char *syntax_filename = NULL;
    char *filename = NULL;
    while(flag != NULL) {
        if(strcmp(flag, "--config") == 0) {
            flag = *argv++;
            if(flag == NULL) {
                fprintf(stderr, "usage: %s --config <config.cano> <filename>\n", program);
                exit(1);
            }
            config_filename = flag;
            flag = *argv++;
        } else {
            filename = flag;
            flag = *argv++;
        }
    }

    initscr();
    noecho();
    raw();

    init_colors();

    // define functions based on current mode
    void(*key_func[MODE_COUNT])(Buffer *buffer, Buffer **modify_buffer, State *state) = {
        handle_normal_keys, handle_insert_keys, handle_search_keys, handle_command_keys, handle_visual_keys
    };

    State state = init_state();
    state.command = calloc(64, sizeof(char));

    getmaxyx(stdscr, state.grow, state.gcol);
    int line_num_width = 5;
    WINDOW *main_win = newwin(state.grow-2, state.gcol-line_num_width, 0, line_num_width);
    WINDOW *line_num_win = newwin(state.grow-2, line_num_width, 0, 0);
    
    state.main_win = main_win;
    state.line_num_win = line_num_win;

    getmaxyx(main_win, state.main_row, state.main_col);
    getmaxyx(line_num_win, state.line_num_row, state.line_num_col);

    WINDOW *status_bar = newwin(2, state.gcol, state.grow-2, 0);
    state.status_bar = status_bar;

    keypad(main_win, TRUE);
    keypad(status_bar, TRUE);


    if(filename == NULL) filename = "out.txt";
    Buffer *buffer = read_file_to_buffer(filename);

    mvwprintw(status_bar, 0, 0, "%.7s", string_modes[mode]);
    wmove(main_win, 0, 0);

    char status_bar_msg[128] = {0};
    state.status_bar_msg = status_bar_msg;

    size_t line_render_start = 0;
    size_t col_render_start = 0;

    push_undo(&state.undo_stack, buffer);

    // load config
    load_config_from_file(&state, buffer, config_filename, syntax_filename);

    while(state.ch != ctrl('q') && QUIT != 1) {
        getmaxyx(state.main_win, state.main_row, state.main_col);

#ifndef NO_CLEAR

        werase(state.main_win);
        werase(state.status_bar);
        werase(line_num_win);

#endif

        int row_s_len = (int)log10(buffer->row_s)+2;
        if(row_s_len > state.line_num_col) {
            wresize(state.main_win, state.main_row, state.main_col-(row_s_len-line_num_width));
            wresize(line_num_win, state.line_num_row, row_s_len);
            mvwin(state.main_win, 0, row_s_len);
            getmaxyx(state.main_win, state.main_row, state.main_col);
            getmaxyx(line_num_win, state.line_num_row, state.line_num_col);
            line_num_width = row_s_len;
        }

        // status bar
        if(state.is_print_msg) {
            mvwprintw(state.status_bar, 1, 0, "%s", state.status_bar_msg);
            wrefresh(state.status_bar);
            sleep(1);
            wclear(state.status_bar);
            state.is_print_msg = 0;
        }

        if(mode == COMMAND || mode == SEARCH) {
            mvwprintw(state.status_bar, 1, 0, ":%.*s", (int)state.command_s, state.command);
        }

        mvwprintw(state.status_bar, 0, 0, "%.7s", string_modes[mode]);
        mvwprintw(state.status_bar, 0, state.gcol/2, "%.3zu:%.3zu", buffer->row_index+1, buffer->cur_pos+1);

        // Adjust the line rendering start based on the current row index
        if(buffer->row_index <= line_render_start) line_render_start = buffer->row_index;
        if(buffer->row_index >= line_render_start+state.main_row) line_render_start = buffer->row_index-state.main_row+1;

        // Adjust the column rendering start based on the current cursor position
        if(buffer->cur_pos <= col_render_start) col_render_start = buffer->cur_pos;
        if(buffer->cur_pos >= col_render_start+state.main_col) col_render_start = buffer->cur_pos-state.main_col+1;

        for(size_t i = line_render_start; i <= line_render_start+state.main_row; i++) {
            if(i <= buffer->row_s) {
                size_t print_index_y = i - line_render_start;

                // Set the line number color to yellow
                wattron(line_num_win, COLOR_PAIR(YELLOW_COLOR));

                if(relative_nums) {
                    if(buffer->row_index == i) {
                        mvwprintw(line_num_win, print_index_y, 0, "%4zu", i+1);
                    }
                    else {
                        mvwprintw(line_num_win, print_index_y, 0, "%4zu", 
                                (size_t)abs((int)i-(int)buffer->row_index));
                    }
                } else {
                    mvwprintw(line_num_win, print_index_y, 0, "%4zu", i+1);
                }

                // Turn off the yellow color
                wattroff(line_num_win, COLOR_PAIR(YELLOW_COLOR));

                size_t off_at = 0;

                // Initialize the token array for syntax highlighting
                size_t token_capacity = 32;
                Token *token_arr = calloc(token_capacity, sizeof(Token));
                size_t token_s = 0;

                // If syntax highlighting is enabled, generate the tokens for the current line
                if(syntax) {
                    token_s = generate_tokens(buffer->rows[i].contents, 
                                                        buffer->rows[i].size, token_arr, &token_capacity);
                }
                
                Color_Pairs color = 0;

            size_t j = 0;
            for(j = col_render_start; j <= col_render_start+state.main_col; j++) {
                size_t keyword_size = 0;
                if(syntax && is_in_tokens_index(token_arr, token_s, j, &keyword_size, 
                                                &color)) {
                    wattron(state.main_win, COLOR_PAIR(color));
                    off_at = j + keyword_size;
                }
                if(syntax && j == off_at) wattroff(state.main_win, COLOR_PAIR(color));
                if(mode == VISUAL) {
                    if(buffer->visual.starting_pos.y == buffer->visual.ending_pos.y) {

                    }
                    Point point = {.x = j, .y = i};
                    int between = 0;
                    if(buffer->visual.starting_pos.y > buffer->visual.ending_pos.y || 
                      (buffer->visual.starting_pos.y == buffer->visual.ending_pos.y &&
                       buffer->visual.starting_pos.x > buffer->visual.ending_pos.x)) {
                        between = is_between(buffer->visual.ending_pos, buffer->visual.starting_pos, point);
                    } else {
                        between = is_between(buffer->visual.starting_pos, buffer->visual.ending_pos, point);
                    }
                    if(between) {
                        wattron(state.main_win, A_STANDOUT);
                    }
                }
                size_t print_index_x = j - col_render_start;
                if(j <= buffer->rows[i].size) {
                    mvwprintw(state.main_win, print_index_y, print_index_x, "%c", buffer->rows[i].contents[j]);
                }
                wattroff(state.main_win, A_STANDOUT);
            }
            free(token_arr);

            }
        }

        wrefresh(state.main_win);
        wrefresh(state.status_bar);
        wrefresh(line_num_win);

        state.repeating.repeating_count = 1;

        state.y = buffer->row_index-line_render_start;
        state.x = buffer->cur_pos-col_render_start;

        if(state.repeating.repeating) {
            mvwprintw(state.status_bar, 1, state.gcol-10, "r");
            wrefresh(state.status_bar);
        }

        if(mode != COMMAND && mode != SEARCH) {
            wmove(state.main_win, state.y, state.x);
            state.ch = wgetch(state.main_win);
        } else if(mode == COMMAND){
            wmove(state.status_bar, 1, buffer->cur_pos+1);
            state.ch = wgetch(state.status_bar);
        } else {
            wmove(state.status_bar, 1, buffer->cur_pos+1);
            state.ch = wgetch(state.status_bar);
        }

        if(state.repeating.repeating) {
            char num[32] = {0};
            size_t num_s = 0;
            while(isdigit(state.ch)) {
                num[num_s++] = state.ch;
                mvwprintw(state.status_bar, 1, (state.gcol-10)+num_s, "%c", num[num_s-1]);
                wrefresh(state.status_bar);
                state.ch = wgetch(state.main_win);
            }
            state.repeating.repeating_count = atoi(num);
            state.repeating.repeating = 0;
        }

        wmove(state.main_win, state.y, state.x);
        state.num_of_braces = num_of_open_braces(buffer);

        for(size_t i = 0; i < state.repeating.repeating_count; i++) {
            key_func[mode](buffer, &buffer, &state);
        }
        if(mode != COMMAND && mode != SEARCH && buffer->cur_pos > buffer->rows[buffer->row_index].size) {
            buffer->cur_pos = buffer->rows[buffer->row_index].size;
        }
        state.x = buffer->cur_pos;
        state.y = buffer->row_index;
        getyx(state.main_win, state.y, state.x);
    }

    endwin();

    free_buffer(&buffer);
    for(size_t i = 0; i < state.undo_stack.buf_capacity; i++) {
        if(state.undo_stack.buf_stack[i] != NULL) {
            free_buffer(&state.undo_stack.buf_stack[i]);
        }
    }
    for(size_t i = 0; i < state.redo_stack.buf_capacity; i++) {
        if(state.redo_stack.buf_stack[i] != NULL) {
            free_buffer(&state.redo_stack.buf_stack[i]);
        }
    }
    return 0;
}
