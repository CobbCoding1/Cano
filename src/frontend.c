#include "frontend.h"

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

int is_between(size_t a, size_t b, size_t c) {
    if(a <= c && c <= b) return 1;
    return 0;
}

void init_colors(State *state) {
    if(has_colors() == FALSE) {
        CRASH("your terminal does not support colors");
    }

    start_color();
    if (state->config.background_color == -1){
        use_default_colors();
    }
    init_pair(YELLOW_COLOR, COLOR_YELLOW, state->config.background_color);
    init_pair(BLUE_COLOR, COLOR_BLUE, state->config.background_color);
    init_pair(GREEN_COLOR, COLOR_GREEN, state->config.background_color);
    init_pair(RED_COLOR, COLOR_RED, state->config.background_color);
    init_pair(MAGENTA_COLOR, COLOR_MAGENTA, state->config.background_color);
    init_pair(CYAN_COLOR, COLOR_CYAN, state->config.background_color);
}

void state_render(State *state) {
    static size_t row_render_start = 0;
    static size_t col_render_start = 0;
    werase(state->main_win);
    werase(state->status_bar);
    werase(state->line_num_win);
    size_t cur_row = buffer_get_row(state->buffer);
    if(state->is_exploring) cur_row = state->explore_cursor;
    Row cur = state->buffer->rows.data[cur_row]; 
    size_t col = state->buffer->cursor - cur.start;
    if(state->is_exploring) col = 0;
    if(cur_row <= row_render_start) row_render_start = cur_row;
    if(cur_row >= row_render_start+state->main_row) row_render_start = cur_row-state->main_row+1;
     if(col <= col_render_start) col_render_start = col;
    if(col >= col_render_start+state->main_col) col_render_start = col-state->main_col+1;
     state->num_of_braces = num_of_open_braces(state->buffer);
    
    if(state->is_print_msg) {
        mvwprintw(state->status_bar, 1, 0, "%s", state->status_bar_msg);    
        wrefresh(state->status_bar);
        sleep(1);
        wclear(state->status_bar);
        state->is_print_msg = 0;
    }
     if (is_term_resized(state->line_num_row, state->line_num_col)) {
        getmaxyx(stdscr, state->grow, state->gcol);
        getmaxyx(state->line_num_win, state->line_num_row, state->line_num_col);			
        mvwin(state->status_bar, state->grow-2, 0);
    }
        
    mvwprintw(state->status_bar, 0, state->gcol/2, "%zu:%zu", cur_row+1, col+1);
    mvwprintw(state->status_bar, 0, state->main_col-11, "%c", state->config.leaders[state->leader]);
    mvwprintw(state->status_bar, 0, 0, "%.7s", string_modes[state->config.mode]);
    mvwprintw(state->status_bar, 0, state->main_col-5, "%.*s", (int)state->num.count, state->num.data);
    
    if(state->config.mode == COMMAND || state->config.mode == SEARCH) mvwprintw(state->status_bar, 1, 0, ":%.*s", (int)state->command_s, state->command);
     if(state->is_exploring) {
        wattron(state->main_win, COLOR_PAIR(BLUE_COLOR));
        for(size_t i = row_render_start; i <= row_render_start+state->main_row; i++) {
            if(i >= state->files->count) break;
             // TODO: All directories should be displayed first, afterwards files
            size_t print_index_y = i - row_render_start;
            mvwprintw(state->main_win, print_index_y, 0, "%s", state->files->data[i].name);
        }
    } else {
        for(size_t i = row_render_start; i <= row_render_start+state->main_row; i++) {
            if(i >= state->buffer->rows.count) break;
            size_t print_index_y = i - row_render_start;
             wattron(state->line_num_win, COLOR_PAIR(YELLOW_COLOR));
            if(state->config.relative_nums) {
                if(cur_row == i) {
                    mvwprintw(state->line_num_win, print_index_y, 0, "%zu", i+1);
                } else {
                    mvwprintw(state->line_num_win, print_index_y, 0, "%zu", (size_t)abs((int)i-(int)cur_row));
                }
            } else {
                mvwprintw(state->line_num_win, print_index_y, 0, "%zu", i+1);
            }
            wattroff(state->line_num_win, COLOR_PAIR(YELLOW_COLOR));
             size_t off_at = 0;
            size_t token_capacity = 32;
            Token *token_arr = calloc(token_capacity, sizeof(Token));
            size_t token_s = 0;
             if(state->config.syntax) {
                token_s = generate_tokens(state->buffer->data.data+state->buffer->rows.data[i].start, 
                                        state->buffer->rows.data[i].end-state->buffer->rows.data[i].start, 
                                        token_arr, &token_capacity);
            }
             Color_Pairs color = 0;
              for(size_t j = state->buffer->rows.data[i].start; j < state->buffer->rows.data[i].end; j++) {
                if(j < state->buffer->rows.data[i].start+col_render_start || j > state->buffer->rows.data[i].end+col+state->main_col) continue;
                size_t col = j-state->buffer->rows.data[i].start;
                size_t print_index_x = col-col_render_start;
                
                for(size_t chr = state->buffer->rows.data[i].start; chr < j; chr++) {
                    if(state->buffer->data.data[chr] == '\t') print_index_x += 3;
                }
                 size_t keyword_size = 0;
                if(state->config.syntax && is_in_tokens_index(token_arr, token_s, col, &keyword_size, &color)) {
                    wattron(state->main_win, COLOR_PAIR(color));
                    off_at = col+keyword_size;
                }
                if(col == off_at) wattroff(state->main_win, COLOR_PAIR(color));
                 if(col > state->buffer->rows.data[i].end) break;
                int between = (state->buffer->visual.start > state->buffer->visual.end) 
                    ? is_between(state->buffer->visual.end, state->buffer->visual.start, state->buffer->rows.data[i].start+col) 
                    : is_between(state->buffer->visual.start, state->buffer->visual.end, state->buffer->rows.data[i].start+col);
                if(state->config.mode == VISUAL && between) wattron(state->main_win, A_STANDOUT);
                else wattroff(state->main_win, A_STANDOUT);
                mvwprintw(state->main_win, print_index_y, print_index_x, "%c", state->buffer->data.data[state->buffer->rows.data[i].start+col]);       
            }
            free(token_arr);
        }
    }
        
    col += count_num_tabs(state->buffer, buffer_get_row(state->buffer))*3;                     
        
    wrefresh(state->main_win);
    wrefresh(state->line_num_win);
    wrefresh(state->status_bar);
     if(state->config.mode == COMMAND || state->config.mode == SEARCH) {
        frontend_move_cursor(state->status_bar, 1, state->x);
        wrefresh(state->status_bar);
    } else {
        frontend_move_cursor(state->main_win, cur_row-row_render_start, col-col_render_start);
    }
}
    
void frontend_init(State *state) {
    initscr();
    noecho();
    raw();

    init_colors(state);

    getmaxyx(stdscr, state->grow, state->gcol);
    int line_num_width = 5;
    WINDOW *main_win = newwin(state->grow-2, state->gcol-line_num_width, 0, line_num_width);
    WINDOW *status_bar = newwin(2, state->gcol, state->grow-2, 0);
    WINDOW *line_num_win = newwin(state->grow-2, line_num_width, 0, 0);
    state->main_win = main_win;
    state->status_bar = status_bar;
    state->line_num_win = line_num_win;
    getmaxyx(main_win, state->main_row, state->main_col);
    getmaxyx(line_num_win, state->line_num_row, state->line_num_col);

    keypad(status_bar, TRUE);
    keypad(main_win, TRUE);
    
    set_escdelay(0);
}
    
void frontend_move_cursor(WINDOW *window, size_t x_pos, size_t y_pos) {
    wmove(window, x_pos, y_pos);
}
    
void frontend_cursor_visible(int value) {
    curs_set(value);
}

int frontend_getch(WINDOW *window) {
    return wgetch(window);
}

void frontend_resize_window(State *state) {
    getmaxyx(stdscr, state->grow, state->gcol);
    wresize(state->main_win, state->grow-2, state->gcol-state->line_num_col);
    wresize(state->status_bar, 2, state->gcol);
    getmaxyx(state->main_win, state->main_row, state->main_col);
    mvwin(state->main_win, 0, state->line_num_col);
    wrefresh(state->main_win);
}