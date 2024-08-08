#include "main.h"

char *string_modes[MODE_COUNT] = {"NORMAL", "INSERT", "SEARCH", "COMMAND", "VISUAL"};
_Static_assert(sizeof(string_modes)/sizeof(*string_modes) == MODE_COUNT, "Number of modes");

char *get_help_page(char *page) {
    if (page == NULL) return NULL;

#ifndef HELP_DIR
    #warning "Missing HELP_DIR definition"

    #define HELP_DIR "docs/help"
#endif

    char *help_page;
    asprintf(&help_page, (HELP_DIR "/%s"), page);

    struct stat file_info;
    if (stat(help_page, &file_info) < 0 || !S_ISREG(file_info.st_mode))
        return NULL;

    return help_page;
}

void handle_flags(char *program, char **argv, int argc, char **config_filename, char **help_filename) {
    char *flag = NULL;

    struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"config", optional_argument, NULL, 'c'},
    };

    char opt = cgetopt_long(argc, argv, "", longopts, NULL);

    while(true) {
        if(opt == -1) break;
        switch(opt) {
            case 'c':
                flag = optarg;
                if(flag == NULL) {
                    fprintf(stderr, "usage: %s --config <config.cano> <filename>\n", program);
                    exit(EXIT_FAILURE);
                }
                *config_filename = flag;
                break;
            case 'h':
                flag = optarg;
                if (flag == NULL) {
                    *help_filename = get_help_page("general");
                } else {
                    *help_filename = get_help_page(flag);
                }

                if (*help_filename == NULL) {
                    fprintf(stderr, "Failed to open help page. Check for typos or if you installed cano properly.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Unexpected flag");
                exit(EXIT_FAILURE);
       }
       opt = cgetopt_long(argc, argv, "", longopts, NULL);
    }
}

/* ------------------------- FUNCTIONS END ------------------------- */

int main(int argc, char **argv) {
    WRITE_LOG("starting (int main)");
    // nice
    setlocale(LC_ALL, "");
    char *program = argv[0];
    char *config_filename = NULL, *syntax_filename = NULL, *help_filename = NULL;
    handle_flags(program, argv, argc, &config_filename, &help_filename);

    char *filename = argv[optind];

    // define functions based on current mode
    void(*key_func[MODE_COUNT])(Buffer *buffer, Buffer **modify_buffer, struct State *state) = {
        handle_normal_keys, handle_insert_keys, handle_search_keys, handle_command_keys, handle_visual_keys
    };

    State state = init_state();
    state.command = calloc(64, sizeof(char));
    state.key_func = key_func;
    state.files = calloc(32, sizeof(File));
    state.config.lang = "UNUSED";
    scan_files(&state, ".");

    frontend_init(&state);

    if(filename == NULL) filename = "out.txt";

    if (help_filename != NULL) {
        state.buffer = load_buffer_from_file(help_filename);
        free(help_filename);
    } else {
        state.buffer = load_buffer_from_file(filename);
    }

    load_config_from_file(&state, state.buffer, config_filename, syntax_filename);

    char status_bar_msg[128] = {0};
    state.status_bar_msg = status_bar_msg;
    buffer_calculate_rows(state.buffer);

    while(state.ch != ctrl('q') && state.config.QUIT != 1) {
        handle_cursor_shape(&state);
        state_render(&state);
        state.ch = frontend_getch(state.main_win);
        state.key_func[state.config.mode](state.buffer, &state.buffer, &state);
    }

    frontend_end();

    free_buffer(state.buffer);
    free_undo_stack(&state.undo_stack);
    free_undo_stack(&state.redo_stack);
    free_files(&state.files);

    free(state.command);
    if(syntax_filename) free(syntax_filename);
    if(config_filename) free(config_filename);

    return 0;
}
