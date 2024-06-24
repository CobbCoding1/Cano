#ifndef VIEW_H
#define VIEW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef struct {
    char *data;
    size_t len;
} String_View;

#define View_Print "%.*s"
#define View_Arg(view) (int)view.len, view.data
#define LITERAL_CREATE(lit) view_create(lit, sizeof(lit)-1)

String_View view_create(char *str, size_t len);
char *view_to_cstr(String_View view);
String_View view_trim_left(String_View view);
String_View view_trim_right(String_View view);
int view_cmp(String_View a, String_View b);
int view_starts_with_c(String_View view, char c);
int view_starts_with_s(String_View a, String_View b);
int view_ends_with_c(String_View view, char c);
int view_ends_with_s(String_View a, String_View b);
int view_contains(String_View haystack, String_View needle);
size_t view_first_of(String_View view, char target);
size_t view_last_of(String_View view, char target);
size_t view_split(String_View view, char c, String_View *arr, size_t arr_s);
String_View view_chop(String_View view, char c);
String_View view_rev(String_View view, char *data, size_t data_s);
size_t view_find(String_View haystack, String_View needle);
int view_to_int(String_View view);
float view_to_float(String_View view);

#endif // VIEW_H
