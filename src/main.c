#include <locale.h>

#include "main.h"

int is_between(size_t a, size_t b, size_t c) {
    if(a <= c && c <= b) return 1;
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

void resize_window(State *state) {
    getmaxyx(stdscr, state->grow, state->gcol);
    wresize(state->main_win, state->grow-2, state->gcol-state->line_num_col);
    wresize(state->status_bar, 2, state->gcol);
    getmaxyx(state->main_win, state->main_row, state->main_col);
    mvwin(state->main_win, 0, state->line_num_col);
    wrefresh(state->main_win);
}

int contains_c_extension(const char *str) {
    const char *extension = ".c";
    size_t str_len = strlen(str);
    size_t extension_len = strlen(extension);

    if (str_len >= extension_len) {
        const char *suffix = str + (str_len - extension_len);
        if (strcmp(suffix, extension) == 0) {
            return 1;
        }
    }

    return 0;
}

void *check_for_errors(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;

    bool loop = 1; /* loop to be used later on, to make it constantly check for errors. Right now it just runs once. */
    while (loop) {

        char path[1035];

        /* Open the command for reading. */
        char command[1024];
        sprintf(command, "gcc %s -o /dev/null -Wall -Wextra -Werror -std=c99 2> errors.cano && echo $? > success.cano", threadArgs->path_to_file);
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            loop = 0;
            static char return_message[] = "Failed to run command";
            WRITE_LOG("Failed to run command");
            return (void *)return_message;
        }
        pclose(fp);

        FILE *should_check_for_errors = fopen("success.cano", "r");

        if (should_check_for_errors == NULL) {
            loop = 0;
            WRITE_LOG("Failed to open file");
            return (void *)NULL;
        }
        while (fgets(path, sizeof(path) -1, should_check_for_errors) != NULL) {
            WRITE_LOG("return code: %s", path);
            if (!(strcmp(path, "0") == 0)) {
                FILE *file_contents = fopen("errors.cano", "r");
                if (fp == NULL) {
                    loop = 0;
                    WRITE_LOG("Failed to open file");
                    return (void *)NULL;
                }

                fseek(file_contents, 0, SEEK_END);
                long filesize = ftell(file_contents);
                fseek(file_contents, 0, SEEK_SET);

                char *buffer = malloc(filesize + 1);
                if (buffer == NULL) {
                    WRITE_LOG("Failed to allocate memory");
                    return (void *)NULL;
                }
                fread(buffer, 1, filesize, file_contents);
                buffer[filesize] = '\0';

                char *bufffer = malloc(filesize + 1);

                while (fgets(path, sizeof(path) -1, file_contents) != NULL) {
                    strcat(bufffer, path);
                    strcat(buffer, "\n");
                }

                char *return_message = malloc(filesize + 1);
                if (return_message == NULL) {
                    WRITE_LOG("Failed to allocate memory");
                    free(buffer);
                    return (void *)NULL;
                }
                strcpy(return_message, buffer);

                free(buffer); 
                loop = 0;
                fclose(file_contents);

                return (void *)return_message;
            }
            else {
                loop = 0;
                static char return_message[] = "No errors found";
                return (void *)return_message;
            }
        }

    }

    return (void *)NULL;
}


Ncurses_Color rgb_to_ncurses(int r, int g, int b) {

    Ncurses_Color color = {0};

    color.r = (int) ((r / 256.0) * 1000);
    color.g = (int) ((g / 256.0) * 1000);
    color.b = (int) ((b / 256.0) * 1000);
    return color;

}

void free_buffer(Buffer *buffer) {
    free(buffer->data.data);
    free(buffer->rows.data);
    free(buffer->filename);
    buffer->data.count = 0;
    buffer->rows.count = 0;
    buffer->data.capacity = 0;
    buffer->rows.capacity = 0;
}

void free_undo(Undo *undo) {
    free(undo->data.data);
}

void free_undo_stack(Undo_Stack *undo) {
    for(size_t i = 0; i < undo->count; i++) {
        free_undo(&undo->data[i]);
    }
    free(undo->data);
}


void init_ncurses_color(int id, int r, int g, int b) {
        Ncurses_Color color = rgb_to_ncurses(r, g, b);
        init_color(id, color.r, color.g, color.b);
}


size_t search(Buffer *buffer, char *command, size_t command_s) {
    for(size_t i = buffer->cursor+1; i < buffer->data.count+buffer->cursor; i++) {
        size_t pos = i % buffer->data.count;
        if(strncmp(buffer->data.data+pos, command, command_s) == 0) {
            // result found and will return the location of the word.
            return pos;
        }
    }
    return buffer->cursor;
}

void replace(Buffer *buffer, State *state, char *new_str, size_t old_str_s, size_t new_str_s) { 
    if (buffer == NULL || new_str == NULL) {
        WRITE_LOG("Error: null pointer");
        return;
    }

    for(size_t i = 0; i < old_str_s; i++) {
        buffer_delete_char(buffer, state);
    }

    for(size_t i = 0; i < new_str_s; i++) {
        buffer_insert_char(state, buffer, new_str[i]);
    }
}


void find_and_replace(Buffer *buffer, State *state, char *old_str, char *new_str) { 
    size_t old_str_s = strlen(old_str);
    size_t new_str_s = strlen(new_str);

    // Search for the old string in the buffer
    size_t position = search(buffer, old_str, old_str_s);
    if (position != (buffer->cursor)) {
        buffer->cursor = position;
        // If the old string is found, replace it with the new string
        replace(buffer, state, new_str, old_str_s, new_str_s);
    }
}

size_t num_of_open_braces(Buffer *buffer) {
    size_t index = buffer->cursor;
    int count = 0;
    int in_str = 0;
    while(index > 0) {
        index--;
        if(buffer->data.data[index] == '"' || buffer->data.data[index] == '\'') in_str = !in_str;
        Brace brace = find_opposite_brace(buffer->data.data[index]);
        if(brace.brace != '0' && !in_str) {
            if(!brace.closing) count++; 
            if(brace.closing) count--; 
        }
    }
    if(count < 0) return 0;
    return count;
}
    
size_t count_num_tabs(Buffer *buffer, size_t row) {
    buffer_calculate_rows(buffer);
    Row cur = buffer->rows.data[row];
    size_t start = cur.start;
    size_t count = 0;
    for(size_t i = start; i < buffer->cursor; i++) {
        if(buffer->data.data[i] == '\t') count++;   
    }
    return count;
}

void reset_command(char *command, size_t *command_s) {
    memset(command, 0, *command_s);
    *command_s = 0;
}


void handle_save(Buffer *buffer) {
    FILE *file = fopen(buffer->filename, "w"); 
    WRITE_LOG("SAVED!");
    fwrite(buffer->data.data, buffer->data.count, sizeof(char), file);
    fclose(file);
}

Buffer *load_buffer_from_file(char *filename) {
    Buffer *buffer = calloc(1, sizeof(Buffer));
    size_t filename_s = strlen(filename)+1; 
    buffer->filename = calloc(filename_s, sizeof(char));
    strncpy(buffer->filename, filename, filename_s);
    FILE *file = fopen(filename, "a+");
    if(file == NULL) CRASH("Could not open file");
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer->data.count = length;
    buffer->data.capacity = (length+1)*2;
    buffer->data.data = calloc(buffer->data.capacity+1, sizeof(char));
	ASSERT(buffer->data.data != NULL, "buffer allocated properly");
    fread(buffer->data.data, length, 1, file);
    fclose(file);
    buffer_calculate_rows(buffer);
    return buffer;
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

void undo_push(State *state, Undo_Stack *stack, Undo undo) {
    DA_APPEND(stack, undo);
    state->cur_undo = (Undo){0};
}

Undo undo_pop(Undo_Stack *stack) {
    if(stack->count <= 0) return (Undo){0};
    return stack->data[--stack->count];
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

void buffer_insert_char(State *state, Buffer *buffer, char ch) {
	ASSERT(buffer != NULL, "buffer exists");
	ASSERT(state != NULL, "state exists");		
    if(buffer->cursor > buffer->data.count) buffer->cursor = buffer->data.count;
    DA_APPEND(&buffer->data, ch);
    memmove(&buffer->data.data[buffer->cursor + 1], &buffer->data.data[buffer->cursor], buffer->data.count - 1 - buffer->cursor);
    buffer->data.data[buffer->cursor] = ch;
    buffer->cursor++;
    state->cur_undo.end = buffer->cursor;
    buffer_calculate_rows(buffer);
}

void buffer_delete_char(Buffer *buffer, State *state) {
    if(buffer->cursor < buffer->data.count) {
        DA_APPEND(&state->cur_undo.data, buffer->data.data[buffer->cursor]);
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

void buffer_yank_line(Buffer *buffer, State *state, size_t offset) {
    size_t row = buffer_get_row(buffer);
    if(offset > index_get_row(buffer, buffer->data.count)) return;
    Row cur = buffer->rows.data[row+offset];
    size_t initial_s = state->clipboard.len;
    state->clipboard.len = cur.end - cur.start + 1; // account for new line
    state->clipboard.str = realloc(state->clipboard.str, 
                                   initial_s+state->clipboard.len*sizeof(char));
    if(state->clipboard.str == NULL) CRASH("null");
    strncpy(state->clipboard.str+initial_s, buffer->data.data+cur.start, state->clipboard.len);
    state->clipboard.len += initial_s;
}

void buffer_yank_char(Buffer *buffer, State *state) {
    reset_command(state->clipboard.str, &state->clipboard.len);
    state->clipboard.len = 2; 
    state->clipboard.str = realloc(state->clipboard.str, 
                                   state->clipboard.len*sizeof(char));
    if(state->clipboard.str == NULL) CRASH("null");
    strncpy(state->clipboard.str, buffer->data.data+buffer->cursor, state->clipboard.len);
}

void buffer_yank_selection(Buffer *buffer, State *state, size_t start, size_t end) {
    state->clipboard.len = end-start+2;
    state->clipboard.str = realloc(state->clipboard.str, 
                                   state->clipboard.len*sizeof(char));
    if(state->clipboard.str == NULL) CRASH("null");
    strncpy(state->clipboard.str, buffer->data.data+start, state->clipboard.len);
}

void buffer_delete_selection(Buffer *buffer, State *state, size_t start, size_t end) {
    int count = end - start;
    buffer->cursor = start;
    while(count >= 0) {
        buffer_delete_char(buffer, state);
        count--;
    }
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

int skip_to_char(Buffer *buffer, int cur_pos, int direction, char c) {
    if(buffer->data.data[cur_pos] == c) {
        cur_pos += direction;
        while(cur_pos > 0 && cur_pos <= (int)buffer->data.count && buffer->data.data[cur_pos] != c) {
            if(cur_pos > 1 && cur_pos < (int)buffer->data.count && buffer->data.data[cur_pos] == '\\') {
                cur_pos += direction;
            }
            cur_pos += direction;
        }
    }
    return cur_pos;
}

void buffer_next_brace(Buffer *buffer) {
    int cur_pos = buffer->cursor;
    Brace initial_brace = find_opposite_brace(buffer->data.data[cur_pos]);
    size_t brace_stack = 0;
    if(initial_brace.brace == '0') return;
    int direction = (initial_brace.closing) ? -1 : 1;
    while(cur_pos >= 0 && cur_pos <= (int)buffer->data.count) {
        cur_pos += direction;
        cur_pos = skip_to_char(buffer, cur_pos, direction, '"');
        cur_pos = skip_to_char(buffer, cur_pos, direction, '\'');
        Brace cur_brace = find_opposite_brace(buffer->data.data[cur_pos]);
        if(cur_brace.brace == '0') continue;
        if((cur_brace.closing && direction == -1) || (!cur_brace.closing && direction == 1)) {
            brace_stack++;
        } else {
            if(brace_stack-- == 0 && cur_brace.brace == find_opposite_brace(initial_brace.brace).brace) {
                buffer->cursor = cur_pos;
                break;
            }
        }
    }
}

int isword(char ch) {
    if(isalnum(ch) || ch == '_') return 1;
    return 0;
}
    
void buffer_create_indent(Buffer *buffer, State *state) {
    if(indent > 0) {
        for(size_t i = 0; i < indent*state->num_of_braces; i++) {
            buffer_insert_char(state, buffer, ' ');
        }
    } else {
        for(size_t i = 0; i < state->num_of_braces; i++) {
            buffer_insert_char(state, buffer, '\t');
        }    
    }
}

void buffer_newline_indent(Buffer *buffer, State *state) {
    buffer_insert_char(state, buffer, '\n');
    buffer_create_indent(buffer, state);
}

int handle_motion_keys(Buffer *buffer, State *state, int ch, size_t *repeating_count) {
    (void)repeating_count;
    switch(ch) {
        case 'g': { // Move to the start of the file or to the line specified by repeating_count
            size_t row = buffer_get_row(buffer);            
            if(state->repeating.repeating_count >= buffer->rows.count) state->repeating.repeating_count = buffer->rows.count;
            if(state->repeating.repeating_count == 0) state->repeating.repeating_count = 1;                
            buffer->cursor = buffer->rows.data[state->repeating.repeating_count-1].start;
            state->repeating.repeating_count = 0;
            if(state->leader != LEADER_D) break;
            // TODO: this doens't work with row jumps
            size_t end = buffer->rows.data[row].end;
            size_t start = buffer->cursor;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'G': { // Move to the end of the file or to the line specified by repeating_count
            size_t row = buffer_get_row(buffer);
            size_t start = buffer->rows.data[row].start;
            if(state->repeating.repeating_count > 0) {
                if(state->repeating.repeating_count >= buffer->rows.count) state->repeating.repeating_count = buffer->rows.count;
                buffer->cursor = buffer->rows.data[state->repeating.repeating_count-1].start;
                state->repeating.repeating_count = 0;                
            } else {
                buffer->cursor = buffer->data.count;   
            }
            if(state->leader != LEADER_D) break;
            // TODO: this doesn't work with row jumps
            size_t end = buffer->cursor;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case '0': { // Move to the start of the line
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->cursor;
            buffer->cursor = buffer->rows.data[row].start;
            if(state->leader != LEADER_D) break;
            size_t start = buffer->cursor;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case '$': { // Move to the end of the line
            size_t row = buffer_get_row(buffer);
            size_t start = buffer->cursor;
            buffer->cursor = buffer->rows.data[row].end;
            if(state->leader != LEADER_D) break;
            size_t end = buffer->cursor;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end-1);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'e': { // Move to the end of the next word
            size_t start = buffer->cursor;
            if(buffer->cursor+1 < buffer->data.count && !isword(buffer->data.data[buffer->cursor+1])) buffer->cursor++;
            while(buffer->cursor+1 < buffer->data.count && 
                (isword(buffer->data.data[buffer->cursor+1]) || isspace(buffer->data.data[buffer->cursor]))
            ) {
                buffer->cursor++;
            }
            if(state->leader != LEADER_D) break;
            size_t end = buffer->cursor;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'b': { // Move to the start of the previous word
            if(buffer->cursor == 0) break;
            size_t end = buffer->cursor;
            if(buffer->cursor-1 > 0 && !isword(buffer->data.data[buffer->cursor-1])) buffer->cursor--;
            while(buffer->cursor-1 > 0 && 
                (isword(buffer->data.data[buffer->cursor-1]) || isspace(buffer->data.data[buffer->cursor+1]))
            ) {
                buffer->cursor--;
            }
            if(buffer->cursor-1 == 0) buffer->cursor--;
            if(state->leader != LEADER_D) break;
            size_t start = buffer->cursor;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'w': { // Move to the start of the next word
            size_t start = buffer->cursor;
            while(buffer->cursor < buffer->data.count && 
                (isword(buffer->data.data[buffer->cursor]) || isspace(buffer->data.data[buffer->cursor+1]))
            ) {
                buffer->cursor++;
            }
            if(buffer->cursor < buffer->data.count) buffer->cursor++;
            if(state->leader != LEADER_D) break;
            size_t end = buffer->cursor-1;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end-1);
            undo_push(state, &state->undo_stack, state->cur_undo);
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
            buffer_next_brace(buffer);
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
        case 'y':
            state->leader = LEADER_Y;
            break;
        default:
            return 0;
    }    
    return 1;
}

int handle_modifying_keys(Buffer *buffer, State *state) {
    switch(state->ch) {
        case 'x': {
            CREATE_UNDO(INSERT_CHARS, buffer->cursor);
            reset_command(state->clipboard.str, &state->clipboard.len);
            buffer_yank_char(buffer, state);
            buffer_delete_char(buffer, state);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'd': {
            switch(state->leader) {
                case LEADER_D: {
                    size_t repeat = state->repeating.repeating_count;
                    if(repeat == 0) repeat = 1;
                    if(repeat > buffer->rows.count - buffer_get_row(buffer)) repeat = buffer->rows.count - buffer_get_row(buffer);
                    for(size_t i = 0; i < repeat; i++) {
                        reset_command(state->clipboard.str, &state->clipboard.len);
                        buffer_yank_line(buffer, state, 0);
                        size_t row = buffer_get_row(buffer);
                        Row cur = buffer->rows.data[row];
                        size_t offset = buffer->cursor - cur.start;
                        CREATE_UNDO(INSERT_CHARS, cur.start);
                        if(row == 0) {
                            buffer_delete_selection(buffer, state, cur.start, cur.end-1);
                        } else {
                            state->cur_undo.start -= 1;
                            buffer_delete_selection(buffer, state, cur.start-1, cur.end-1);
                        }
                        undo_push(state, &state->undo_stack, state->cur_undo);
                        buffer_calculate_rows(buffer);
                        if(row >= buffer->rows.count) row = buffer->rows.count-1;
                        cur = buffer->rows.data[row];
                        size_t pos = cur.start + offset;
                        if(pos > cur.end) pos = cur.end;
                        buffer->cursor = pos;
                    }
                    state->repeating.repeating_count = 0;
                } break;
                default:
                    break;
            }
        } break;
        case 'r': {
            CREATE_UNDO(REPLACE_CHAR, buffer->cursor);
            DA_APPEND(&state->cur_undo.data, buffer->data.data[buffer->cursor]);
            state->ch = wgetch(state->main_win); 
            buffer->data.data[buffer->cursor] = state->ch;
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        default: {
            return 0;
        }
    }
    return 1;
}

int handle_normal_to_insert_keys(Buffer *buffer, State *state) {
    switch(state->ch) {
        case 'i': {
            mode = INSERT;
        } break;
        case 'I': {
            size_t row = buffer_get_row(buffer);
            Row cur = buffer->rows.data[row];
            buffer->cursor = cur.start;            
            while(buffer->cursor < cur.end && isspace(buffer->data.data[buffer->cursor])) buffer->cursor++;
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
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->rows.data[row].end;
            buffer->cursor = end;
            buffer_newline_indent(buffer, state);
            mode = INSERT;
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'O': {
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            size_t row = buffer_get_row(buffer);
            size_t start = buffer->rows.data[row].start;
            buffer->cursor = start;
            buffer_newline_indent(buffer, state);
            mode = INSERT;
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        default: {
            return 0;
        }
    }
    CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
    return 1;
}
    
int check_keymaps(Buffer *buffer, State *state) {
    (void)buffer;
    for(size_t i = 0; i < key_maps.count; i++) {
        if(state->ch == key_maps.data[i].a) {
            for(size_t j = 0; j < key_maps.data[i].b_s; j++) {
                state->ch = key_maps.data[i].b[j];
                state->key_func[mode](buffer, &buffer, state);   
            }
            return 1;
        }
    }
    return 0;
}

int compare_name(File const *leftp, File const *rightp)
{
    return strcoll(leftp->name, rightp->name);
}

void scan_files(Files *files, char *directory) {
    DIR *dp = opendir(directory);
    if(dp == NULL) {
        WRITE_LOG("Failed to open directory: %s\n", directory);
        CRASH("Failed to open directory");
    }

    struct dirent *dent;
    while((dent = readdir(dp)) != NULL) {
        // Do not ignore .. in order to navigate back to the last directory
        if(strcmp(dent->d_name, ".") == 0) continue;
        
        char *path = calloc(256, sizeof(char));
        strcpy(path, directory);
        strcat(path, "/");
        strcat(path, dent->d_name);
        
        char *name = calloc(256, sizeof(char));
        strcpy(name, dent->d_name);

        if(dent->d_type == DT_DIR) {
            strcat(name, "/");
            DA_APPEND(files, ((File){name, path, true}));
        } else if(dent->d_type == DT_REG) {
            DA_APPEND(files, ((File){name, path, false}));
        }
    }
    closedir(dp);
    qsort(files->data, files->count,
        sizeof *files->data, (__compar_fn_t)&compare_name);
}

void free_files(Files *files) {
    for(size_t i = 0; i < files->count; ++i) {
        free(files->data[i].name);
        free(files->data[i].path);
    }
    free(files);
}

void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    
    if(check_keymaps(buffer, state)) return;
    if(state->leader == LEADER_NONE && handle_leader_keys(state)) return;   
    if(isdigit(state->ch) && !(state->ch == '0' && state->num.count == 0)) {
        DA_APPEND(&state->num, state->ch);
        return;
    } 
    
    if(!isdigit(state->ch) && state->num.count > 0) {
        ASSERT(state->num.data, "num is not initialized");
        state->repeating.repeating_count = atoi(state->num.data);
        if(state->repeating.repeating_count == 0) return;
        state->num.count = 0;
        for(size_t i = 0; i < state->repeating.repeating_count; i++) {
            state->key_func[mode](buffer, modify_buffer, state);
        }
        state->repeating.repeating_count = 0;
        memset(state->num.data, 0, state->num.capacity);
        return;
    }

    switch(state->ch) {
        case ':':
            state->x = 1;
            wmove(state->status_bar, state->x, 1);
            mode = COMMAND;
            break;
        case '/':
            if(state->is_exploring) break;
            reset_command(state->command, &state->command_s);        
            state->x = state->command_s+1;
            wmove(state->status_bar, state->x, 1);
            mode = SEARCH;
            break;
        case 'v':
            if(state->is_exploring) break;
            buffer->visual.start = buffer->cursor;
            buffer->visual.end = buffer->cursor;
            buffer->visual.is_line = 0;
            mode = VISUAL;
            break;
        case 'V':
            if(state->is_exploring) break;
            buffer->visual.start = buffer->rows.data[buffer_get_row(buffer)].start;
            buffer->visual.end = buffer->rows.data[buffer_get_row(buffer)].end;
            buffer->visual.is_line = 1;
            mode = VISUAL;
            break;
        case ctrl('o'): {
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            buffer_insert_char(state, buffer, '\n');
            buffer_create_indent(buffer, state);
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'n': {
            size_t index = search(buffer, state->command, state->command_s);
            buffer->cursor = index;
        } break;
        case 'u': {
            Undo undo = undo_pop(&state->undo_stack); 
            Undo redo = {0};
            redo.start = undo.start;
            state->cur_undo = redo;
            switch(undo.type) {
                case NONE:
                    break;
                case INSERT_CHARS:
                    state->cur_undo.type = (undo.data.count > 1) ? DELETE_MULT_CHAR : DELETE_CHAR;
                    state->cur_undo.end = undo.start + undo.data.count;
                    buffer->cursor = undo.start;
                    for(size_t i = 0; i < undo.data.count; i++) {
                        buffer_insert_char(state, buffer, undo.data.data[i]);
                    } 
                    break;
                case DELETE_CHAR:
                    state->cur_undo.type = INSERT_CHARS;
                    buffer->cursor = undo.start;
                    buffer_delete_char(buffer, state);
                    break;
                case DELETE_MULT_CHAR:
                    state->cur_undo.type = INSERT_CHARS;
                    state->cur_undo.end = undo.end;
                    buffer->cursor = undo.start;
                    for(size_t i = undo.start; i < undo.end; i++) {
                        buffer_delete_char(buffer, state);
                    }
                    break;
                case REPLACE_CHAR:
                    state->cur_undo.type = REPLACE_CHAR;
                    buffer->cursor = undo.start;
                    DA_APPEND(&undo.data, buffer->data.data[buffer->cursor]);
                    buffer->data.data[buffer->cursor] = undo.data.data[0]; 
                    break;
            }
            undo_push(state, &state->redo_stack, state->cur_undo);
            free_undo(&undo);
        } break;
        case 'U': {
            Undo redo = undo_pop(&state->redo_stack); 
            Undo undo = {0};
            undo.start = redo.start;
            state->cur_undo = undo;
            switch(redo.type) {
                case NONE:
                    return;
                    break;
                case INSERT_CHARS:
                    state->cur_undo.type = (redo.data.count > 1) ? DELETE_MULT_CHAR : DELETE_CHAR;
                    state->cur_undo.end = redo.start + redo.data.count;
                    buffer->cursor = redo.start;
                    for(size_t i = 0; i < redo.data.count; i++) {
                        buffer_insert_char(state, buffer, redo.data.data[i]);
                    } 
                    break;
                case DELETE_CHAR:
                    state->cur_undo.type = INSERT_CHARS;
                    buffer->cursor = redo.start;
                    buffer_delete_char(buffer, state);
                    break;
                case DELETE_MULT_CHAR:
                    state->cur_undo.type = INSERT_CHARS;
                    state->cur_undo.end = redo.end;
                    buffer->cursor = undo.start;
                    for(size_t i = redo.start; i < redo.end; i++) {
                        buffer_delete_char(buffer, state);
                    }
                    break;
                case REPLACE_CHAR:
                    state->cur_undo.type = REPLACE_CHAR;
                    buffer->cursor = redo.start;
                    DA_APPEND(&redo.data, buffer->data.data[buffer->cursor]);
                    buffer->data.data[buffer->cursor] = redo.data.data[0]; 
                    break;
            }
            undo_push(state, &state->undo_stack, state->cur_undo);
            free_undo(&redo);
        } break;
        case ctrl('s'): {
            handle_save(buffer);
            QUIT = 1;
        } break;
        case ESCAPE:
            state->repeating.repeating_count = 0;
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case KEY_RESIZE: {
            resize_window(state);
        } break;
        case 'y': {
            switch(state->leader) {
                case LEADER_Y: {
                    if(state->repeating.repeating_count == 0) state->repeating.repeating_count = 1;
                    reset_command(state->clipboard.str, &state->clipboard.len);
                    for(size_t i = 0; i < state->repeating.repeating_count; i++) {
                        buffer_yank_line(buffer, state, i);
                    }
                    state->repeating.repeating_count = 0;
                } break;
                default:
                    break;
            }
        } break;
        case 'p': {
            if(state->clipboard.len == 0) break;
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            for(size_t i = 0; i < state->clipboard.len-1; i++) {
                buffer_insert_char(state, buffer, state->clipboard.str[i]);
            }
            state->cur_undo.end = buffer->cursor;
            undo_push(state, &state->undo_stack, state->cur_undo); 
        } break;
        case ctrl('n'): {
            state->is_exploring = !state->is_exploring;
        } break;
        default: {
            if(state->is_exploring) {
                switch(state->ch) {
                    case DOWN_ARROW:
                    case 'j': // Move down
                        if (state->explore_cursor < state->files->count-1) {
                            state->explore_cursor++;
                        }
                        break;
                    case UP_ARROW:
                    case 'k': // Move up
                        if (state->explore_cursor > 0) {
                            state->explore_cursor--;
                        }
                        break;
                    case ENTER: {
                        File f = state->files->data[state->explore_cursor];
                        if (f.is_directory) {
                            char str[256];
                            strcpy(str, f.path);

                            free_files(state->files);
                            state->files = calloc(32, sizeof(File));
                            scan_files(state->files, str);

                            state->explore_cursor = 0;
                        } else {
                            // TODO: Load syntax highlighting right here
                            state->buffer = load_buffer_from_file(f.path);
                            state->is_exploring = false;
                        }
                    } break;
                }
                break;
            }

            if(handle_modifying_keys(buffer, state)) break;
            if(handle_motion_keys(buffer, state, state->ch, &state->repeating.repeating_count)) break;
            if(handle_normal_to_insert_keys(buffer, state)) break;
        } break;
    }
    if(state->repeating.repeating_count == 0) {
        state->leader = LEADER_NONE;
    }
}

void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
	ASSERT(buffer, "buffer exists");
	ASSERT(state, "state exists");
	
    switch(state->ch) { 
        case '\b':
        case 127:
        case KEY_BACKSPACE: { 
            if(buffer->cursor > 0) {
                buffer->cursor--;
                buffer_delete_char(buffer, state);
            }
        } break;
        case ctrl('s'): {
            handle_save(buffer);
            QUIT = 1;
        } break;
        case ESCAPE: // Switch to NORMAL mode
            //state->cur_undo.end = buffer->cursor;
            if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
            mode = NORMAL;
            break;
        case LEFT_ARROW: { // Move cursor left
            state->cur_undo.end = buffer->cursor;
            if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
            buffer_move_left(buffer);
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
        } break;
        case DOWN_ARROW: { // Move cursor down
            state->cur_undo.end = buffer->cursor;
            if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
            buffer_move_down(buffer);
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
        } break;
        case UP_ARROW: {   // Move cursor up
            state->cur_undo.end = buffer->cursor;
            if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
            buffer_move_up(buffer);
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
        } break;
        case RIGHT_ARROW: { // Move cursor right
            state->cur_undo.end = buffer->cursor;
            if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
            buffer_move_right(buffer);
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
        } break;
        case KEY_RESIZE: {
            resize_window(state);
        } break;
        case KEY_TAB:
            if(indent > 0) {
                for(size_t i = 0; (int)i < indent; i++) {
                    buffer_insert_char(state, buffer, ' ');
                }
            } else {
                buffer_insert_char(state, buffer, '\t');
            }
            break;
        case KEY_ENTER:
        case ENTER: {
            if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            Brace brace = find_opposite_brace(buffer->data.data[buffer->cursor]);
            buffer_newline_indent(buffer, state);
            if(brace.brace != '0' && brace.closing) {
                buffer_insert_char(state, buffer, '\n');
                if(state->num_of_braces == 0) state->num_of_braces = 1;
                if(indent > 0) {
                    for(size_t i = 0; i < indent*(state->num_of_braces-1); i++) {
                        buffer_insert_char(state, buffer, ' ');
                        buffer->cursor--;
                    }
                } else {
                    for(size_t i = 0; i < state->num_of_braces-1; i++) {
                        buffer_insert_char(state, buffer, '\t');                            
                        buffer->cursor--;
                    }        
                }
                buffer->cursor--;
            }
        } break;
        default: { // Handle other characters
			ASSERT(buffer->data.count >= buffer->cursor, "check");
			ASSERT(buffer->data.data, "check2");
			Brace brace = (Brace){0};
			if(buffer->cursor > 0) {
	            Brace cur_brace = find_opposite_brace(buffer->data.data[buffer->cursor]);
	            if((cur_brace.brace != '0' && cur_brace.closing && 
	                state->ch == find_opposite_brace(cur_brace.brace).brace)) {
	                buffer->cursor++;
	                break;
	            };
	            brace = find_opposite_brace(state->ch);
			}
            // TODO: make quotes auto close
            buffer_insert_char(state, buffer, state->ch);
            if(brace.brace != '0' && !brace.closing) {
                buffer_insert_char(state, buffer, brace.brace);
	            undo_push(state, &state->undo_stack, state->cur_undo);
                buffer->cursor--;
	            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);				
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
            if(state->x != 1) {
                shift_str_left(state->command, &state->command_s, --state->x);
                wmove(state->status_bar, 1, state->x);
            }
        } break;
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case ctrl('s'): {
            handle_save(buffer);
            QUIT = 1;
        } break;
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
                size_t command_s = 0;
                Command_Token *command = lex_command(view_create(state->command, state->command_s), &command_s);
                execute_command(buffer, state, command, command_s);
            }
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
        } break;
        case LEFT_ARROW:
            if(state->x > 1) state->x--;
            break;
        case DOWN_ARROW:
            break;
        case UP_ARROW:
            break;
        case RIGHT_ARROW:
            if(state->x < state->command_s) state->x++;
            break;
        case KEY_RESIZE:
            resize_window(state);
            break;
        default: {
            shift_str_right(state->command, &state->command_s, state->x);
            state->command[(state->x++)-1] = state->ch;
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
            if(state->x != 1) {
                shift_str_left(state->command, &state->command_s, --state->x);
                wmove(state->status_bar, 1, state->x);
            }
        } break;
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            mode = NORMAL;
            break;
        case ENTER: {
            size_t index = search(buffer, state->command, state->command_s);
            if(state->command_s > 2 && strncmp(state->command, "s/", 2) == 0) {
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
                    count++;

                    // log for args.
                    token = strtok(NULL, "/");
                }
                index = search(buffer, args[0], strlen(args[0]));
                find_and_replace(buffer, state, args[0], args[1]);
            } 
            buffer->cursor = index;
            mode = NORMAL;
        } break;
        case ctrl('s'): {
            handle_save(buffer);
            QUIT = 1;
        } break;
        case LEFT_ARROW:
            if(state->x > 1) state->x--;
            break;
        case DOWN_ARROW:
            break;
        case UP_ARROW:
            break;
        case RIGHT_ARROW:
            if(state->x < state->command_s) state->x++;
            break;
        case KEY_RESIZE:
            resize_window(state);
            break;
        default: {
            shift_str_right(state->command, &state->command_s, state->x);
            state->command[(state->x++)-1] = state->ch;
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
        case ctrl('s'): {
            handle_save(buffer);
            QUIT = 1;
        } break;
        case 'd':
        case 'x': {
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
            mode = NORMAL;
            curs_set(1);
        } break;
        case '>': {
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            size_t position = buffer->cursor + indent;
            size_t row = index_get_row(buffer, start);
            size_t end_row = index_get_row(buffer, end);
            for(size_t i = row; i <= end_row; i++) {
                buffer_calculate_rows(buffer);
                buffer->cursor = buffer->rows.data[i].start;
                if(indent > 0) {
                    for(size_t i = 0; (int)i < indent; i++) {
                        buffer_insert_char(state, buffer, ' ');
                    }
                } else {
                     buffer_insert_char(state, buffer, '\t');                           
                }
            }
            buffer->cursor = position;
            mode = NORMAL;
            curs_set(1);
        } break;
        case '<': {
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            size_t row = index_get_row(buffer, start);
            size_t end_row = index_get_row(buffer, end);            
            size_t offset = 0;
            for(size_t i = row; i <= end_row; i++) {
                buffer_calculate_rows(buffer);                
                buffer->cursor = buffer->rows.data[i].start;
                if(indent > 0) {
                    for(size_t j = 0; (int)j < indent; j++) {
                        if(isspace(buffer->data.data[buffer->cursor])) {
                            buffer_delete_char(buffer, state);
                            offset++;
                            buffer_calculate_rows(buffer);
                        }
                    }
                } else {
                    if(isspace(buffer->data.data[buffer->cursor])) {
                        buffer_delete_char(buffer, state);
                        offset++;
                        buffer_calculate_rows(buffer);
                    }
                }
            }
            mode = NORMAL;
            curs_set(1);
        } break;
        case 'y': {
            reset_command(state->clipboard.str, &state->clipboard.len);
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            buffer_yank_selection(buffer, state, start, end);
            buffer->cursor = start;
            mode = NORMAL;
            curs_set(1);
            break;
        }
        default: {
            handle_motion_keys(buffer, state, state->ch, &state->repeating.repeating_count);
            if(buffer->visual.is_line) {
                buffer->visual.end = buffer->rows.data[buffer_get_row(buffer)].end;
                if(buffer->visual.start >= buffer->visual.end) {
                    buffer->visual.end = buffer->rows.data[buffer_get_row(buffer)].start;
                    buffer->visual.start = buffer->rows.data[index_get_row(buffer, buffer->visual.start)].end;
                }
            } else {
                buffer->visual.end = buffer->cursor;
            }
        } break;
    }
}
    
State init_state() {
    State state = {0};
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
            size_t cmd_s = 0;
            Command_Token *cmd = lex_command(view_create(lines[i], strlen(lines[i])), &cmd_s);
            execute_command(buffer, state, cmd, cmd_s);
            free(lines[i]);
        }
    }
    free(lines);

    if(syntax_filename != NULL) {
        Color_Arr color_arr = parse_syntax_file(syntax_filename);
        if(color_arr.arr != NULL) {
            for(size_t i = 0; i < color_arr.arr_s; i++) {
                init_pair(color_arr.arr[i].custom_slot, color_arr.arr[i].custom_id, background_color);
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
    if (background_color == -1){
        use_default_colors();
    }
    init_pair(YELLOW_COLOR, COLOR_YELLOW, background_color);
    init_pair(BLUE_COLOR, COLOR_BLUE, background_color);
    init_pair(GREEN_COLOR, COLOR_GREEN, background_color);
    init_pair(RED_COLOR, COLOR_RED, background_color);
    init_pair(MAGENTA_COLOR, COLOR_MAGENTA, background_color);
    init_pair(CYAN_COLOR, COLOR_CYAN, background_color);
}

void print_help_page(char *page) {
    if (page == NULL) {
        return;
    }

    char *env = getenv("HOME");
    if(env == NULL) CRASH("could not get HOME");
    char help_page[128];
    snprintf(help_page, 128, "%s/.local/share/cano/help/%s", env, page);

    FILE *page_file = fopen(help_page, "r");
    if (page_file == NULL) {
        fprintf(stderr, "Failed to open help page. Check for typos or if you installed cano properly.\n");
        exit(EXIT_FAILURE);
    }

    char str[256];
    while (fgets(str, 256, page_file)) {
        printf("%s", str);
    }
    fclose(page_file);
}

/* ------------------------- FUNCTIONS END ------------------------- */


int main(int argc, char **argv) {

    WRITE_LOG("starting (int main)");
    setlocale(LC_ALL, "");

    (void)argc;
    char *program = *argv++;
    char *flag = *argv++;
    char *config_filename = NULL;
    char *syntax_filename = NULL;
    char *filename = NULL;
    while(flag != NULL) {
        bool isC = 0;
        if (!(filename == NULL)) {
            isC = contains_c_extension(filename);
            if(isC) {
                WRITE_LOG("C file detected");
                lang = "C";
            }
        }

        if (strcmp(lang, "C") == 0) {
            pthread_t thread1;
            int process;

            ThreadArgs args = {
                .path_to_file = filename,
                .lang = "C"
            };

            process = pthread_create(&thread1, NULL, check_for_errors, &args);
            if(process) {
                WRITE_LOG("Error - pthread_create() return code: %d", process);
                exit(EXIT_FAILURE);
            }
        }

        if(strcmp(flag, "--config") == 0) {
            flag = *argv++;
            if(flag == NULL) {
                fprintf(stderr, "usage: %s --config <config.cano> <filename>\n", program);
                exit(1);
            }
            config_filename = flag;
        } else if (strcmp(flag, "--help") == 0) {
            flag = *argv++;
            if (flag == NULL) {
                print_help_page("general");
            } else {
                print_help_page(flag);
            }
            exit(EXIT_SUCCESS);
        } else {
            filename = flag;
        }
        flag = *argv++;
    }
    (void)config_filename;
    (void)syntax_filename;

    // define functions based on current mode
    void(*key_func[MODE_COUNT])(Buffer *buffer, Buffer **modify_buffer, struct State *state) = {
        handle_normal_keys, handle_insert_keys, handle_search_keys, handle_command_keys, handle_visual_keys
    };

    State state = init_state();
    state.command = calloc(64, sizeof(char));
    state.key_func = key_func;
    state.files = calloc(32, sizeof(File));
    scan_files(state.files, ".");

    initscr();
    noecho();
    raw();

    init_colors();

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
    state.buffer = load_buffer_from_file(filename);
    
    load_config_from_file(&state, state.buffer, config_filename, syntax_filename);

    char status_bar_msg[128] = {0};
    state.status_bar_msg = status_bar_msg;
    buffer_calculate_rows(state.buffer);

    size_t row_render_start = 0;
    size_t col_render_start = 0;

    while(state.ch != ctrl('q') && QUIT != 1) {
        werase(main_win);
        werase(status_bar);
        werase(line_num_win);
        size_t cur_row = buffer_get_row(state.buffer);
        if(state.is_exploring) cur_row = state.explore_cursor;
        Row cur = state.buffer->rows.data[cur_row]; 
        size_t col = state.buffer->cursor - cur.start;
        if(state.is_exploring) col = 0;
        if(cur_row <= row_render_start) row_render_start = cur_row;
        if(cur_row >= row_render_start+state.main_row) row_render_start = cur_row-state.main_row+1;

        if(col <= col_render_start) col_render_start = col;
        if(col >= col_render_start+state.main_col) col_render_start = col-state.main_col+1;

        state.num_of_braces = num_of_open_braces(state.buffer);
        
        if(state.is_print_msg) {
            mvwprintw(state.status_bar, 1, 0, "%s", state.status_bar_msg);    
            wrefresh(state.status_bar);
            sleep(1);
            wclear(state.status_bar);
            state.is_print_msg = 0;
        }

        if (is_term_resized(state.line_num_row, state.line_num_col)) {
            getmaxyx(stdscr, state.grow, state.gcol);
            mvwin(state.status_bar, state.grow-2, 0);
        }
            
        mvwprintw(status_bar, 0, state.gcol/2, "%zu:%zu", cur_row+1, col+1);
        mvwprintw(status_bar, 0, state.main_col-11, "%c", leaders[state.leader]);
        mvwprintw(state.status_bar, 0, 0, "%.7s", string_modes[mode]);
        mvwprintw(state.status_bar, 0, state.main_col-5, "%.*s", (int)state.num.count, state.num.data);
        
        if(mode == COMMAND || mode == SEARCH) mvwprintw(state.status_bar, 1, 0, ":%.*s", (int)state.command_s, state.command);

        if(state.is_exploring) {
            wattron(state.main_win, COLOR_PAIR(BLUE_COLOR));
            for(size_t i = row_render_start; i <= row_render_start+state.main_row; i++) {
                if(i >= state.files->count) break;

                // TODO: All directories should be displayed first, afterwards files
                size_t print_index_y = i - row_render_start;
                mvwprintw(state.main_win, print_index_y, 0, "%s", state.files->data[i].name);
            }
        } else {
            for(size_t i = row_render_start; i <= row_render_start+state.main_row; i++) {
                if(i >= state.buffer->rows.count) break;
                size_t print_index_y = i - row_render_start;

                wattron(state.line_num_win, COLOR_PAIR(YELLOW_COLOR));
                if(relative_nums) {
                    if(cur_row == i) {
                        mvwprintw(state.line_num_win, print_index_y, 0, "%zu", i+1);
                    } else {
                        mvwprintw(state.line_num_win, print_index_y, 0, "%zu", (size_t)abs((int)i-(int)cur_row));
                    }
                } else {
                    mvwprintw(state.line_num_win, print_index_y, 0, "%zu", i+1);
                }
                wattroff(state.line_num_win, COLOR_PAIR(YELLOW_COLOR));

                size_t off_at = 0;
                size_t token_capacity = 32;
                Token *token_arr = calloc(token_capacity, sizeof(Token));
                size_t token_s = 0;

                if(syntax) {
                    token_s = generate_tokens(state.buffer->data.data+state.buffer->rows.data[i].start, 
                                            state.buffer->rows.data[i].end-state.buffer->rows.data[i].start, 
                                            token_arr, &token_capacity);
                }

                Color_Pairs color = 0;


                for(size_t j = state.buffer->rows.data[i].start; j < state.buffer->rows.data[i].end; j++) {
                    if(j < state.buffer->rows.data[i].start+col_render_start || j > state.buffer->rows.data[i].end+col+state.main_col) continue;
                    size_t col = j-state.buffer->rows.data[i].start;
                    size_t print_index_x = col-col_render_start;
                    
                    for(size_t chr = state.buffer->rows.data[i].start; chr < j; chr++) {
                        if(state.buffer->data.data[chr] == '\t') print_index_x += 3;
                    }

                    size_t keyword_size = 0;
                    if(syntax && is_in_tokens_index(token_arr, token_s, col, &keyword_size, &color)) {
                        wattron(state.main_win, COLOR_PAIR(color));
                        off_at = col+keyword_size;
                    }
                    if(col == off_at) wattroff(state.main_win, COLOR_PAIR(color));

                    if(col > state.buffer->rows.data[i].end) break;
                    int between = (state.buffer->visual.start > state.buffer->visual.end) 
                        ? is_between(state.buffer->visual.end, state.buffer->visual.start, state.buffer->rows.data[i].start+col) 
                        : is_between(state.buffer->visual.start, state.buffer->visual.end, state.buffer->rows.data[i].start+col);
                    if(mode == VISUAL && between) wattron(state.main_win, A_STANDOUT);
                    else wattroff(state.main_win, A_STANDOUT);
                    mvwprintw(state.main_win, print_index_y, print_index_x, "%c", state.buffer->data.data[state.buffer->rows.data[i].start+col]);       
                }
                free(token_arr);
            }
        }
            
        col += count_num_tabs(state.buffer, buffer_get_row(state.buffer))*3;                     
            
        wrefresh(state.main_win);
        wrefresh(state.line_num_win);
        wrefresh(state.status_bar);

        if(mode == COMMAND || mode == SEARCH) {
            wmove(state.status_bar, 1, state.x);
            wrefresh(state.status_bar);
        } else {
            wmove(state.main_win, cur_row-row_render_start, col-col_render_start);
        }

        state.ch = wgetch(main_win);
        state.key_func[mode](state.buffer, &state.buffer, &state);
    }
    endwin();

    free_buffer(state.buffer);
    free_undo_stack(&state.undo_stack);
    free_undo_stack(&state.redo_stack);
    free_files(state.files);

    return 0;
}