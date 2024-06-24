#include "tools.h"

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
    
void reset_command(char *command, size_t *command_s) {
    memset(command, 0, *command_s);
    *command_s = 0;
}

void free_undo_stack(Undo_Stack *undo) {
    for(size_t i = 0; i < undo->count; i++) {
        free_undo(&undo->data[i]);
    }
    free(undo->data);
}


void handle_save(Buffer *buffer) {
    FILE *file = fopen(buffer->filename, "w"); 
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
		default:
		    return (Brace){.brace = '0'};		
    }
}

    
int check_keymaps(Buffer *buffer, State *state) {
    (void)buffer;
    for(size_t i = 0; i < state->config.key_maps.count; i++) {
        if(state->ch == state->config.key_maps.data[i].a) {
            for(size_t j = 0; j < state->config.key_maps.data[i].b_s; j++) {
                state->ch = state->config.key_maps.data[i].b[j];
                state->key_func[state->config.mode](buffer, &buffer, state);   
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
    
void load_config_from_file(State *state, Buffer *buffer, char *config_filename, char *syntax_filename) {
    if(config_filename == NULL) {
        char default_config_filename[128] = {0};
        char config_dir[64] = {0};
		if(!state->env) {
			state->env = calloc(128, sizeof(char));
	        char *env = getenv("HOME");			
	        if(env == NULL) CRASH("could not get HOME");			
			state->env = env;
		}
        sprintf(config_dir, "%s/.config/cano", state->env);
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
            Command_Token *cmd = lex_command(state, view_create(lines[i], strlen(lines[i])), &cmd_s);
            execute_command(buffer, state, cmd, cmd_s);
            free(lines[i]);
        }
    }
    free(lines);

    if(syntax_filename != NULL) {
        Color_Arr color_arr = parse_syntax_file(syntax_filename);
        if(color_arr.arr != NULL) {
            for(size_t i = 0; i < color_arr.arr_s; i++) {
                init_pair(color_arr.arr[i].custom_slot, color_arr.arr[i].custom_id, state->config.background_color);
                init_ncurses_color(color_arr.arr[i].custom_id, color_arr.arr[i].custom_r, 
                                   color_arr.arr[i].custom_g, color_arr.arr[i].custom_b);
            }

            free(color_arr.arr);
        }
    }
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
    
void init_ncurses_color(int id, int r, int g, int b) {
        Ncurses_Color color = rgb_to_ncurses(r, g, b);
        init_color(id, color.r, color.g, color.b);
}
