#include "keys.h"
#include "main.h"

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

void motion_g(State *state) {
    size_t row = buffer_get_row(state->buffer);
    if(state->repeating.repeating_count >= state->buffer->rows.count)
            state->repeating.repeating_count = state->buffer->rows.count;
    if(state->repeating.repeating_count == 0) state->repeating.repeating_count = 1;
    state->buffer->cursor = state->buffer->rows.data[state->repeating.repeating_count-1].start;
    state->repeating.repeating_count = 0;
    if(state->leader != LEADER_D) return;
    // TODO: this doens't work with row jumps
    size_t end = state->buffer->rows.data[row].end;
    size_t start = state->buffer->cursor;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(state->buffer, state, start, end);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

void motion_G(State *state) {
    size_t row = buffer_get_row(state->buffer);
    size_t start = state->buffer->rows.data[row].start;
    if(state->repeating.repeating_count > 0) {
        if(state->repeating.repeating_count >= state->buffer->rows.count)
            state->repeating.repeating_count = state->buffer->rows.count;
        state->buffer->cursor = state->buffer->rows.data[state->repeating.repeating_count-1].start;
        state->repeating.repeating_count = 0;
    } else {
        state->buffer->cursor = state->buffer->data.count;
    }
    if(state->leader != LEADER_D) return;
    // TODO: this doesn't work with row jumps
    size_t end = state->buffer->cursor;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(state->buffer, state, start, end);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

void motion_0(State *state) {
    size_t row = buffer_get_row(state->buffer);
    size_t end = state->buffer->cursor;
    state->buffer->cursor = state->buffer->rows.data[row].start;
    if(state->leader != LEADER_D) return;
    size_t start = state->buffer->cursor;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(state->buffer, state, start, end);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

void motion_$(State *state) {
    size_t row = buffer_get_row(state->buffer);
    size_t start = state->buffer->cursor;
    state->buffer->cursor = state->buffer->rows.data[row].end;
    if(state->leader != LEADER_D) return;
    size_t end = state->buffer->cursor;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(state->buffer, state, start, end-1);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

void motion_e(State *state) {
    size_t start = state->buffer->cursor;
    if(state->buffer->cursor+1 < state->buffer->data.count &&
       !isword(state->buffer->data.data[state->buffer->cursor+1])) state->buffer->cursor++;
    while(state->buffer->cursor+1 < state->buffer->data.count &&
        (isword(state->buffer->data.data[state->buffer->cursor+1]) ||
          isspace(state->buffer->data.data[state->buffer->cursor]))
    ) {
        state->buffer->cursor++;
    }
    if(state->leader != LEADER_D) return;
    size_t end = state->buffer->cursor;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(state->buffer, state, start, end);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

void motion_b(State *state) {
    Buffer *buffer = state->buffer;
    if(buffer->cursor == 0) return;
    size_t end = buffer->cursor;
    if(buffer->cursor-1 > 0 && !isword(buffer->data.data[buffer->cursor-1])) buffer->cursor--;
    while(buffer->cursor-1 > 0 &&
        (isword(buffer->data.data[buffer->cursor-1]) || isspace(buffer->data.data[buffer->cursor+1]))
    ) {
        buffer->cursor--;
    }
    if(buffer->cursor-1 == 0) buffer->cursor--;
    if(state->leader != LEADER_D) return;
    size_t start = buffer->cursor;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(buffer, state, start, end);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

void motion_w(State *state) {
    Buffer *buffer = state->buffer;
    size_t start = buffer->cursor;
    while(buffer->cursor < buffer->data.count &&
        (isword(buffer->data.data[buffer->cursor]) || isspace(buffer->data.data[buffer->cursor+1]))
    ) {
        buffer->cursor++;
    }
    if(buffer->cursor < buffer->data.count) buffer->cursor++;
    if(state->leader != LEADER_D) return;
    size_t end = buffer->cursor-1;
    CREATE_UNDO(INSERT_CHARS, start);
    buffer_delete_selection(buffer, state, start, end-1);
    undo_push(state, &state->undo_stack, state->cur_undo);
}

int handle_motion_keys(Buffer *buffer, State *state, int ch, size_t *repeating_count) {
    (void)repeating_count;
    switch(ch) {
        case 'g': { // Move to the start of the file or to the line specified by repeating_count
            motion_g(state);
        } break;
        case 'G': { // Move to the end of the file or to the line specified by repeating_count
            motion_G(state);
        } break;
        case '0': { // Move to the start of the line
            motion_0(state);
        } break;
        case '$': { // Move to the end of the line
            motion_$(state);
        } break;
        case 'e': { // Move to the end of the next word
            motion_e(state);
        } break;
        case 'b': { // Move to the start of the previous word
            motion_b(state);
        } break;
        case 'w': { // Move to the start of the next word
            motion_w(state);
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
            buffer_delete_ch(buffer, state);
        } break;
        case 'd': {
            switch(state->leader) {
                case LEADER_D: {
                    buffer_delete_row(buffer, state);
                }
                default: break;
            }
        } break;
        case 'r': {
            buffer_replace_ch(buffer, state);
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
			state->config.mode = INSERT;		
			if(state->repeating.repeating_count) {
				state->ch = frontend_getch(state->main_win);
			}
        } break;
        case 'I': {
            size_t row = buffer_get_row(buffer);
            Row cur = buffer->rows.data[row];
            buffer->cursor = cur.start;
            while(buffer->cursor < cur.end && isspace(buffer->data.data[buffer->cursor])) buffer->cursor++;
            state->config.mode = INSERT;
        } break;
        case 'a':
            if(buffer->cursor < buffer->data.count) buffer->cursor++;
            state->config.mode = INSERT;
            break;
        case 'A': {
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->rows.data[row].end;
            buffer->cursor = end;
            state->config.mode = INSERT;
        } break;
        case 'o': {
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->rows.data[row].end;
            buffer->cursor = end;
            buffer_newline_indent(buffer, state);
            state->config.mode = INSERT;
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        case 'O': {
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            size_t row = buffer_get_row(buffer);
            size_t start = buffer->rows.data[row].start;
            buffer->cursor = start;
            buffer_newline_indent(buffer, state);
            state->config.mode = INSERT;
            undo_push(state, &state->undo_stack, state->cur_undo);
        } break;
        default: {
            return 0;
        }
    }
    CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
    return 1;
}

void buffer_handle_undo(State *state, Undo *undo) {
    Buffer *buffer = state->buffer;
    Undo redo = {0};
    redo.start = undo->start;
    state->cur_undo = redo;
    switch(undo->type) {
        case NONE:
            break;
        case INSERT_CHARS:
            state->cur_undo.type = (undo->data.count > 1) ? DELETE_MULT_CHAR : DELETE_CHAR;
            state->cur_undo.end = undo->start + undo->data.count-1;
            buffer->cursor = undo->start;
            buffer_insert_selection(buffer, &undo->data, undo->start);
            break;
        case DELETE_CHAR:
            state->cur_undo.type = INSERT_CHARS;
            buffer->cursor = undo->start;
            buffer_delete_char(buffer, state);
            break;
        case DELETE_MULT_CHAR:
            state->cur_undo.type = INSERT_CHARS;
            state->cur_undo.end = undo->end;
            buffer->cursor = undo->start;
            WRITE_LOG("%zu %zu", undo->start, undo->end);
            buffer_delete_selection(buffer, state, undo->start, undo->end);
            break;
        case REPLACE_CHAR:
            state->cur_undo.type = REPLACE_CHAR;
            buffer->cursor = undo->start;
            DA_APPEND(&undo->data, buffer->data.data[buffer->cursor]);
            buffer->data.data[buffer->cursor] = undo->data.data[0];
            break;
    }
}
	
void handle_explore_keys(State *state) {
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
                      free_files(&state->files);
                     state->files = calloc(32, sizeof(File));
                     scan_files(state, str);
                     state->explore_cursor = 0;
                 } else {
                     free_buffer(state->buffer);
                     state->buffer = load_buffer_from_file(f.path);
					 //char *config_filename = NULL;
					 //char *syntax_filename = NULL;							
					 // TODO: Make this config work
	 				//load_config_from_file(state, state->buffer, config_filename, syntax_filename);			
	             	state->is_exploring = false;
            }
       } break;
	}
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
            state->key_func[state->config.mode](buffer, modify_buffer, state);
        }
        state->repeating.repeating_count = 0;
        memset(state->num.data, 0, state->num.capacity);
        return;
    }

    switch(state->ch) {
        case ':':
            state->x = 1;
            frontend_move_cursor(state->status_bar, state->x, 1);
            state->config.mode = COMMAND;
            break;
        case '/':
            if(state->is_exploring) break;
            reset_command(state->command, &state->command_s);
            state->x = state->command_s+1;
            frontend_move_cursor(state->status_bar, state->x, 1);
            state->config.mode = SEARCH;
            break;
        case 'v':
            if(state->is_exploring) break;
            buffer->visual.start = buffer->cursor;
            buffer->visual.end = buffer->cursor;
            buffer->visual.is_line = 0;
            state->config.mode = VISUAL;
            break;
        case 'V':
            if(state->is_exploring) break;
            buffer->visual.start = buffer->rows.data[buffer_get_row(buffer)].start;
            buffer->visual.end = buffer->rows.data[buffer_get_row(buffer)].end;
            buffer->visual.is_line = 1;
            state->config.mode = VISUAL;
            break;
        case ctrl('o'): {
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            size_t row = buffer_get_row(buffer);
            size_t end = buffer->rows.data[row].end;
            buffer->cursor = end;
            buffer_newline_indent(buffer, state);
            undo_push(state, &state->undo_stack, state->cur_undo);
            break;
        } break;
        case 'n': {
            size_t index = search(buffer, state->command, state->command_s);
            buffer->cursor = index;
        } break;
        case 'u': {
            Undo undo = undo_pop(&state->undo_stack);
            buffer_handle_undo(state, &undo);
            undo_push(state, &state->redo_stack, state->cur_undo);
            free_undo(&undo);
        } break;
        case 'U': {
            Undo redo = undo_pop(&state->redo_stack);
            buffer_handle_undo(state, &redo);
            undo_push(state, &state->undo_stack, state->cur_undo);
            free_undo(&redo);
        } break;
        case ctrl('s'): {
            handle_save(buffer);
            state->config.QUIT = 1;
        } break;
        case ctrl('c'):
        case ESCAPE:
            state->repeating.repeating_count = 0;
            reset_command(state->command, &state->command_s);
            state->config.mode = NORMAL;
            break;
        case KEY_RESIZE: {
            frontend_resize_window(state);
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
            Data data = dynstr_to_data(state->clipboard);
            if(state->clipboard.len > 0 && state->clipboard.str[0] == '\n') {
                WRITE_LOG("newline");
                size_t row = buffer_get_row(buffer);
                size_t end = buffer->rows.data[row].end;
                buffer->cursor = end;
            }
            buffer_insert_selection(buffer, &data, buffer->cursor);
            state->cur_undo.end = buffer->cursor;
            undo_push(state, &state->undo_stack, state->cur_undo);
            if(state->clipboard.len > 0 && state->clipboard.str[0] == '\n' && buffer->cursor < buffer->data.count)
                    buffer->cursor++;
        } break;
        case ctrl('n'): {
            state->is_exploring = !state->is_exploring;
        } break;
        default: {
			if(state->is_exploring) handle_explore_keys(state);
            if(handle_modifying_keys(buffer, state)) break;
            if(handle_motion_keys(buffer, state, state->ch, &state->repeating.repeating_count)) break;
            if(handle_normal_to_insert_keys(buffer, state)) break;
        } break;
    }
    if(state->repeating.repeating_count == 0) {
        state->leader = LEADER_NONE;
    }
}

void handle_move_left(State *state, size_t num) {
    state->cur_undo.end = state->buffer->cursor;
    if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
    for(size_t i = 0; i < num; i++) {
        buffer_move_left(state->buffer);                    
    }
    CREATE_UNDO(DELETE_MULT_CHAR, state->buffer->cursor);
}
    
void handle_move_down(State *state, size_t num) {
    state->cur_undo.end = state->buffer->cursor;
    if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
    for(size_t i = 0; i < num; i++) {
        buffer_move_down(state->buffer);                    
    }
    CREATE_UNDO(DELETE_MULT_CHAR, state->buffer->cursor);
}
    
void handle_move_up(State *state, size_t num) {
    state->cur_undo.end = state->buffer->cursor;
    if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
    for(size_t i = 0; i < num; i++) {
        buffer_move_up(state->buffer);                    
    }
    CREATE_UNDO(DELETE_MULT_CHAR, state->buffer->cursor);
}
    
void handle_move_right(State *state, size_t num) {
    state->cur_undo.end = state->buffer->cursor;
    if(state->cur_undo.end != state->cur_undo.start) undo_push(state, &state->undo_stack, state->cur_undo);
    for(size_t i = 0; i < num; i++) {
        buffer_move_right(state->buffer);                    
    }
    CREATE_UNDO(DELETE_MULT_CHAR, state->buffer->cursor);
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
            state->config.QUIT = 1;
        } break;
        case ctrl('c'):
        case ESCAPE: // Switch to NORMAL mode
            state->cur_undo.end = buffer->cursor;
            undo_push(state, &state->undo_stack, state->cur_undo);
            state->config.mode = NORMAL;
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);        
            break;
        case LEFT_ARROW: { // Move cursor left
            handle_move_left(state, 1);                                
        } break;
        case DOWN_ARROW: { // Move cursor down
            handle_move_down(state, 1);                                        
        } break;
        case UP_ARROW: {   // Move cursor up
            handle_move_up(state, 1);                                                
        } break;
        case RIGHT_ARROW: { // Move cursor right
            handle_move_right(state, 1);                                                        
        } break;
        case KEY_RESIZE: {
            frontend_resize_window(state);
        } break;
        case KEY_TAB:
			buffer_create_indent(buffer, state, 1);
			break;
        case KEY_ENTER:
        case ENTER: {
            state->cur_undo.end = buffer->cursor;                                    
            undo_push(state, &state->undo_stack, state->cur_undo);

            Brace brace = find_opposite_brace(buffer->data.data[buffer->cursor]);
            CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);
            buffer_newline_indent(buffer, state);
            state->cur_undo.end = buffer->cursor;                            

			// handle the extra brace which gets inserted for brace auto-close
			buffer_brace_indent(state, brace);
        } break;
        default: { // Handle other characters
			ASSERT(buffer->data.count >= buffer->cursor, "check");
			ASSERT(buffer->data.data, "check2");
			Brace brace = (Brace){0};
			Brace cur_brace = {0};
			if(buffer->cursor == 0) cur_brace = find_opposite_brace(buffer->data.data[0]);			
			else cur_brace = find_opposite_brace(buffer->data.data[buffer->cursor]);
			
            if((cur_brace.brace != '0' && cur_brace.closing &&
                state->ch == find_opposite_brace(cur_brace.brace).brace)) {
                buffer->cursor++;
                break;
            };
            brace = find_opposite_brace(state->ch);
            // TODO: make quotes auto close
            buffer_insert_char(state, buffer, state->ch);
            if(brace.brace != '0' && !brace.closing) {
                state->cur_undo.end -= 1;
                undo_push(state, &state->undo_stack, state->cur_undo);
                CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor-1);
                buffer_insert_char(state, buffer, brace.brace);
	            undo_push(state, &state->undo_stack, state->cur_undo);
                CREATE_UNDO(DELETE_MULT_CHAR, buffer->cursor);                                                    
                handle_move_left(state, 1);                                                            
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
                frontend_move_cursor(state->status_bar, 1, state->x);
            }
        } break;
        case ctrl('c'):
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            state->config.mode = NORMAL;
            break;
        case ctrl('s'): {
            handle_save(buffer);
            state->config.QUIT = 1;
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
                Command_Token *command = lex_command(state, view_create(state->command, state->command_s), &command_s);
                execute_command(buffer, state, command, command_s);
            }
            reset_command(state->command, &state->command_s);
            state->config.mode = NORMAL;
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
            frontend_resize_window(state);
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
                frontend_move_cursor(state->status_bar, 1, state->x);
            }
        } break;
        case ctrl('c'):
        case ESCAPE:
            reset_command(state->command, &state->command_s);
            state->config.mode = NORMAL;
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
            state->config.mode = NORMAL;
        } break;
        case ctrl('s'): {
            handle_save(buffer);
            state->config.QUIT = 1;
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
            frontend_resize_window(state);
            break;
        default: {
            shift_str_right(state->command, &state->command_s, state->x);
            state->command[(state->x++)-1] = state->ch;
        } break;
    }
}

void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state) {
    (void)modify_buffer;
    frontend_cursor_visible(0);
    switch(state->ch) {
        case ctrl('c'):
        case ESCAPE: {
            state->config.mode = NORMAL;
            frontend_cursor_visible(1);
            state->buffer->visual = (Visual){0};
        } break;
        case ENTER: break;
        case ctrl('s'): {
            handle_save(buffer);
            state->config.QUIT = 1;
        } break;
        case 'd':
        case 'x': {
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            CREATE_UNDO(INSERT_CHARS, start);
            buffer_delete_selection(buffer, state, start, end);
            undo_push(state, &state->undo_stack, state->cur_undo);
            state->config.mode = NORMAL;
            frontend_cursor_visible(1);
        } break;
        case '>': {
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            size_t position = buffer->cursor + state->config.indent;
            size_t row = index_get_row(buffer, start);
            size_t end_row = index_get_row(buffer, end);
            for(size_t i = row; i <= end_row; i++) {
                buffer_calculate_rows(buffer);
                buffer->cursor = buffer->rows.data[i].start;
				buffer_create_indent(buffer, state, 1);
            }
            buffer->cursor = position;
            state->config.mode = NORMAL;
            frontend_cursor_visible(1);
        } break;
        case '<': {
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            size_t row = index_get_row(buffer, start);
            size_t end_row = index_get_row(buffer, end);
            for(size_t i = row; i <= end_row; i++) {
                buffer_calculate_rows(buffer);
                buffer->cursor = buffer->rows.data[i].start;
                if(state->config.indent > 0) {
					buffer_delete_indent(state, state->config.indent);
                } else {
					buffer_delete_indent(state, 1);

                }
            }
            state->config.mode = NORMAL;
            frontend_cursor_visible(1);
        } break;
        case 'y': {
            reset_command(state->clipboard.str, &state->clipboard.len);
            int cond = (buffer->visual.start > buffer->visual.end);
            size_t start = (cond) ? buffer->visual.end : buffer->visual.start;
            size_t end = (cond) ? buffer->visual.start : buffer->visual.end;
            buffer_yank_selection(buffer, state, start, end);
            buffer->cursor = start;
            state->config.mode = NORMAL;
            frontend_cursor_visible(1);
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
