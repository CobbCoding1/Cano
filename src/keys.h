#ifndef KEYS_H
#define KEYS_H

#include "defs.h"
#include "frontend.h"
#include "tools.h"
#include "buffer.h"

int handle_motion_keys(Buffer *buffer, State *state, int ch, size_t *repeating_count);
int handle_modifying_keys(Buffer *buffer, State *state);
int handle_normal_to_insert_keys(Buffer *buffer, State *state);
void handle_normal_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_insert_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_command_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_search_keys(Buffer *buffer, Buffer **modify_buffer, State *state);
void handle_visual_keys(Buffer *buffer, Buffer **modify_buffer, State *state);

#endif
