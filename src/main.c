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

#ifdef REFACTOR

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

#endif

size_t num_of_open_braces(Buffer *buffer) {
    size_t index = buffer->cursor;
    int count = 0;
    while(index > 0) {
        index--;
        Brace brace = find_opposite_brace(buffer->data.data[index]);
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


#ifdef REFACTOR
void handle_save(Buffer *buffer) {
    FILE *file = fopen(buffer->filename, "w"); 
    for(size_t i = 0; i <= buffer->row_s; i++) {
        fwrite(buffer->rows[i].contents, buffer->rows[i].size, 1, file);
        fwrite("\n", sizeof("\n")-1, 1, file);
    }
    fclose(file);
}
#endif


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


#ifdef REFACTOR
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
#endif

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

void buffer_calculate_rows(Buffer *buffer) {
    buffer->rows.count = 0;
    size_t start = 0;
    for(size_t i = 0; i < buffer->data.count; i++) {
        if(buffer->data.data[i] == '\n') {
            DA_APPEND(&buffer->rows, ((Row){.start = start, .end = i}));
            start = i + 1;
        }
    }


    DA_APPEND(&buffer->rows, ((Row){.start = start, .end = buffer->data.count}));
}

void buffer_insert_char(Buffer *buffer, char ch) {
    if(buffer->cursor > buffer->data.count) buffer->cursor = buffer->data.count;
    DA_APPEND(&buffer->data, ch);
    memmove(&buffer->data.data[buffer->cursor + 1], &buffer->data.data[buffer->cursor], buffer->data.count - 1 - buffer->cursor);
    buffer->data.data[buffer->cursor] = ch;
    buffer->cursor++;
    buffer_calculate_rows(buffer);
}

void buffer_delete_char(Buffer *buffer) {
    if(buffer->cursor < buffer->data.count) {
        memmove(&buffer->data.data[buffer->cursor], &buffer->data.data[buffer->cursor+1], buffer->data.count - buffer->cursor - 1);
        buffer->data.count--;
        buffer_calculate_rows(buffer);
    }
}

size_t buffer_get_row(const Buffer *buffer) {
    ASSERT(buffer->cursor <= buffer->data.count, "cursor: %zu", buffer->cursor);
    ASSERT(buffer->rows.count >= 1, "there must be at least one line");
    for(size_t i = 0; i < buffer->rows.count; i++) {
        if(buffer->rows.data[i].start <= buffer->cursor && buffer->cursor <= buffer->rows.data[i].end) {
            return i;
        }
    }
    return 0;
}

size_t index_get_row(Buffer *buffer, size_t index) {
    ASSERT(index <= buffer->data.count, "index: %zu", index);
    ASSERT(buffer->rows.count >= 1, "there must be at least one line");
    for(size_t i = 0; i < buffer->rows.count; i++) {
        if(buffer->rows.data[i].start <= index && index <= buffer->rows.data[i].end) {
            return i;
        }
    }
    return 0;
}

void buffer_delete_row(Buffer *buffer) {
    size_t row = buffer_get_row(buffer);
    Row cur = buffer->rows.data[row];
    buffer->cursor = (row == 0) ? cur.start : cur.start-1;
    memmove(&buffer->data.data[buffer->cursor], &buffer->data.data[buffer->cursor+cur.end-buffer->cursor], buffer->data.count - buffer->cursor - 1);
    buffer->data.count -= cur.end-cur.start;
    buffer_calculate_rows(buffer);
}

void buffer_move_up(Buffer *buffer) {
    size_t row = buffer_get_row(buffer);
    size_t col = buffer->cursor - buffer->rows.data[row].start;
    if(row > 0) {
        buffer->cursor = buffer->rows.data[row-1].start + col;
        if(buffer->cursor > buffer->rows.data[row-1].end) {
            buffer->cursor = buffer->rows.data[row-1].end;
        }
    }
}

void buffer_move_down(Buffer *buffer) {
    size_t row = buffer_get_row(buffer);
    size_t col = buffer->cursor - buffer->rows.data[row].start;
    if(row < buffer->rows.count - 1) {
        buffer->cursor = buffer->rows.data[row+1].start + col;
        if(buffer->cursor > buffer->rows.data[row+1].end) {
            buffer->cursor = buffer->rows.data[row+1].end;
        }
    }
}

void buffer_move_right(Buffer *buffer) {
    if(buffer->cursor < buffer->data.count) buffer->cursor++;
}

void buffer_move_left(Buffer *buffer) {
    if(buffer->cursor > 0) buffer->cursor--;
}

#ifdef REFACTOR
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

    size_t final_s = *str_s - index;
    char *temp = calloc(final_s, sizeof(char));
    if(temp == NULL) {
        CRASH("no more ram");
    }
    size_t temp_len = 0;
    for(size_t i = index; i < *str_s; i++) {
        temp[temp_len++] = buf->rows[dest_index-1].contents[i];
        buf->rows[dest_index-1].contents[i] = '\0';
    }
    shift_rows_right(buf, dest_index);
    Row *cur = &buf->rows[dest_index];
    if(cur->capacity < final_s) resize_row(&cur, final_s*2);
    strncpy(cur->contents, temp, sizeof(char)*final_s);
    buf->rows[dest_index].size = final_s;
    *str_s = index;
    free(temp);
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
#endif

int handle_motion_keys(Buffer *buffer, int ch, size_t *repeating_count) {
    (void)repeating_count;
    switch(ch) {
        case 'g': // Move to the start of the file or to the line specified by repeating_count
            break;
        case 'G': // Move to the end of the file or to the line specified by repeating_count
            break;
        case '0': // Move to the start of the line
            break;
        case '$': // Move to the end of the line
            break;
        case 'e': { // Move to the end of the next word
        } break;
        case 'b': { // Move to the start of the previous word
        } break;
        case 'w': { // Move to the start of the next word
        } break;
        case LEFT_ARROW:
        case 'h': // Move left
            buffer_move_left(buffer);
            break;
        case DOWN_ARROW:
        case 'j': // Move down
            buffer_move_down(buffer);
            break;
        case UP_ARROW:
        case 'k': // Move up
            buffer_move_up(buffer);
            break;
        case RIGHT_ARROW:
        case 'l': // Move right
            buffer_move_right(buffer);
            break;
        case '%': { // Move to the matching brace
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

int handle_modifying_keys(Buffer *buffer, State *state) {
    switch(state->ch) {
        case 'x': {
            buffer_delete_char(buffer);
        } break;
        case 'w': {
        } break;
        case 'd': {
            switch(state->leader) {
                case LEADER_D:
                    buffer_delete_row(buffer);
                    break;
                default:
                    break;
            }
        } break;
        case 'r': {
            state->ch = wgetch(state->main_win); 
            buffer->data.data[buffer->cursor] = state->ch;
        } break;
        default: {
            return 0;
        }
    }
    return 1;
}

int handle_normal_to_insert_keys(Buffer *buffer, State *state) {
    (void)buffer;
    switch(state->ch) {
        case 'i':
            mode = INSERT;
            break;
        case 'I': {
            size_t row = buffer_get_row(buffer);
            size_t start = buffer->rows.data[row].start;
            buffer->cursor = start;
            mode = INSERT;
        } break;
        case 'a':
            if(buffer->cursor < buffer->data.count) buffer->cursor++;
            mode = INSERT;
            break;
        case 'A': {
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->rows.data[row].end;
            buffer->cursor = end;
            mode = INSERT;
        } break;
        case 'o': {
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->rows.data[row].end;
            buffer->cursor = end;
            buffer_insert_char(buffer, '\n');
            mode = INSERT;
        } break;
        case 'O': {
            size_t row = buffer_get_row(buffer);
            size_t start = buffer->rows.data[row].start;
            buffer->cursor = start;
            buffer_insert_char(buffer, '\n');
            buffer->cursor = start;
            mode = INSERT;
        } break;
        default: {
            return 0;
        }
    }
    return 1;
}

void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    if(state->leader == LEADER_NONE && handle_leader_keys(state)) {
        return;   
    } 
    switch(state->ch) {
        case ':':
            mode = COMMAND;
            break;
        case '/':
            mode = SEARCH;
            break;
        case 'v':
            mode = VISUAL;
            break;
        case 'V':
            mode = VISUAL;
            break;
        case ctrl('o'): {
        } break;
        case 'R':
            break;
        case 'n': {
        } break;
        case 'u': {
        } break;
        case 'U': {
        } break;
        case ctrl('s'): {
            QUIT = 1;
        } break;
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case KEY_RESIZE: {
        } break;
        default: {
            handle_motion_keys(buffer, state->ch, &state->repeating.repeating_count);
            handle_normal_to_insert_keys(buffer, state);
            handle_modifying_keys(buffer, state);
        } break;
    }
    state->leader = LEADER_NONE;
}

void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)buffer;
    (void)modify_buffer;
    switch(state->ch) { 
        case '\b':
        case 127:
        case KEY_BACKSPACE: { 
            if(buffer->cursor > 0) {
                buffer->cursor--;
                buffer_delete_char(buffer);
            }
        } break;
        case ESCAPE: // Switch to NORMAL mode
            mode = NORMAL;
            break;
        case LEFT_ARROW: // Move cursor left
            buffer_move_left(buffer);
            break;
        case DOWN_ARROW: // Move cursor down
            buffer_move_down(buffer);
            break;
        case UP_ARROW:   // Move cursor up
            buffer_move_up(buffer);
            break;
        case RIGHT_ARROW: // Move cursor right
            buffer_move_right(buffer);
            break;
        case KEY_RESIZE: {
        } break;
        default: { // Handle other characters
            buffer_insert_char(buffer, state->ch);
        } break;
    }
}

void handle_command_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    (void)buffer;
    switch(state->ch) {
        case '\b':
        case 127:
        case KEY_BACKSPACE: {
        } break;
        case ESCAPE:
            mode = NORMAL;
            break;
        case KEY_ENTER:
        case ENTER: {
            mode = NORMAL;
        } break;
        case LEFT_ARROW:
            break;
        case DOWN_ARROW:
            break;
        case UP_ARROW:
            break;
        case RIGHT_ARROW:
            break;
        default: {
        } break;
    }
}

void handle_search_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    (void)buffer;
    switch(state->ch) {
        case '\b':
        case 127:
        case KEY_BACKSPACE: {
        } break;
        case ESCAPE:
            mode = NORMAL;
            break;
        case ENTER: {
            mode = NORMAL;
        } break;
        case LEFT_ARROW:
            break;
        case DOWN_ARROW:
            break;
        case UP_ARROW:
            break;
        case RIGHT_ARROW:
            break;
        default: {
        } break;
    }
}

void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    (void)buffer;
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
            mode = NORMAL;
            curs_set(1);
            break;
        default: {
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

#ifdef REFACTOR
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
#endif

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
    (void)config_filename;
    (void)syntax_filename;

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
    WINDOW *status_bar = newwin(2, state.gcol, state.grow-2, 0);
    WINDOW *line_num_win = newwin(state.grow-2, line_num_width, 0, 0);
    state.main_win = main_win;
    state.status_bar = status_bar;
    state.line_num_win = line_num_win;
    getmaxyx(main_win, state.main_row, state.main_col);
    getmaxyx(line_num_win, state.line_num_row, state.line_num_col);

    keypad(status_bar, TRUE);
    keypad(main_win, TRUE);

    if(filename == NULL) filename = "out.txt";
    Buffer *buffer = calloc(1, sizeof(Buffer));

    char status_bar_msg[128] = {0};
    state.status_bar_msg = status_bar_msg;
    buffer_calculate_rows(buffer);

    while(state.ch != ctrl('q') && QUIT != 1) {
        werase(main_win);
        werase(status_bar);
        werase(line_num_win);
        size_t cur_row = buffer_get_row(buffer);

        size_t col = buffer->cursor - buffer->rows.data[cur_row].start;

        mvwprintw(status_bar, 0, state.gcol/2, "%zu:%zu", cur_row+1, col+1);

        for(size_t i = 0; i < buffer->data.count; i++) {
            size_t row = index_get_row(buffer, i);
            size_t col = i - buffer->rows.data[row].start;
            mvwprintw(state.main_win, row, col, "%c", buffer->data.data[i]);
            if(relative_nums) {
                if(cur_row == row) {
                    mvwprintw(state.line_num_win, row, 0, "%zu", row+1);
                } else {
                    mvwprintw(state.line_num_win, row, 0, "%zu", (size_t)abs((int)row-(int)cur_row));
                }
            } else {
                mvwprintw(state.line_num_win, row, 0, "%zu", row+1);
            }
        }

        mvwprintw(state.status_bar, 0, 0, "%.7s", string_modes[mode]);

        wmove(state.main_win, cur_row, col);

        wrefresh(state.status_bar);
        wrefresh(state.line_num_win);
        wrefresh(state.main_win);

        state.ch = wgetch(main_win);
        key_func[mode](buffer, &buffer, &state);
    }
    endwin();
    return 0;


    #ifdef REFACTOR
    // load config
    //load_config_from_file(&state, buffer, config_filename, syntax_filename);

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
    #endif
    return 0;
}
