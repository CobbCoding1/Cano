#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "view.h"

int view_cmp(String_View a, String_View b) {
    return (a.len != b.len) ? 0 : !strncmp(a.data, b.data, a.len);
}

char *view_to_cstr(String_View view) {
    char *str = malloc(sizeof(char) * view.len+1);
    strncpy(str, view.data, view.len);
    str[view.len] = '\0';
    return str;
}

String_View view_trim_left(String_View view) {
    size_t i = 0;

    while(i < view.len && isspace(view.data[i]))
        i++;
    return view_create(view.data + i, view.len - i);
}

String_View view_trim_right(String_View view) {
    size_t i = view.len - 1;

    while(i < view.len && isspace(view.data[i]))
        i--;
    return view_create(view.data, i + 1);
}

int view_contains(String_View haystack, String_View needle) {
    if(needle.len > haystack.len) return 0;
    String_View compare = view_create(haystack.data, needle.len);
    for(size_t i = 0; i < haystack.len; i++) {
        compare.data = haystack.data + i;
        if(view_cmp(needle, compare))
            return 1;
    }
    return 0;
}

size_t view_first_of(String_View view, char target) {
    for(size_t i = 0; i < view.len; i++)
        if(view.data[i] == target)
            return i;
    return 0;
}

size_t view_last_of(String_View view, char target) {
    for(size_t i = view.len-1; i > 0; i--)
        if(view.data[i] == target)
            return i;
    return 0;
}

size_t view_split(String_View view, char c, String_View *arr, size_t arr_s) {
    char *cur = view.data;
    size_t arr_index = 0;
    size_t i;

    for(i = 0; i < view.len; i++) {
        if(view.data[i] == c) {
            if(arr_index < arr_s-2) {
                arr[arr_index++] = view_create(cur, view.data + i - cur);
                cur = view.data + i + 1;
            } else {
                arr[arr_index++] = view_create(view.data + i+1, view.len - i-1);
                return arr_index;
            }
        }
    }
    arr[arr_index++] = view_create(cur, view.data + i - cur);
    return arr_index;
}

String_View view_chop(String_View view, char c) {
    size_t i = 0;

    while(view.data[i] != c && i != view.len)
        i++;
    if(i < view.len)
        i++;
    return view_create(view.data + i, view.len - i);
}

String_View view_rev(String_View view, char *data, size_t data_s) {
    if(view.len >= data_s)
        return view_create(NULL, 0);
    String_View result = view_create(data, view.len);

    for(int i = view.len-1; i >= 0; i--)
        result.data[view.len-1 - i] = view.data[i];
    return result;
}

size_t view_find(String_View haystack, String_View needle) {
    if(needle.len > haystack.len) return 0;
    String_View compare = view_create(haystack.data, needle.len);
    for(size_t i = 0; i < haystack.len; i++) {
        compare.data = haystack.data + i;
        if(view_cmp(needle, compare))
            return i;
    }
    return 0;
}

int view_to_int(String_View view) {
    return strtol(view.data, NULL, 10);
}

float view_to_float(String_View view) {
    return strtof(view.data, NULL);
}
