#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "str.h"

string mk_empty_string() {
    string s;
    s.size = 0;
    s.capacity = 16;
    s.data = (char *)malloc(s.capacity * sizeof(char));
    s.data[0] = '\0';
    return s;
}

string mk_string(const char *fmt, ...) {
    string s = mk_empty_string();
    va_list args;
    va_start(args, fmt);
    string_printf(&s, fmt, args);
    va_end(args);
    return s;
}

void string_printf(string *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    s->size = 0;
    string_printfa(s, fmt, args);
    va_end(args);
}

void string_printfa(string *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int new_size = s->size + snprintf(s->data, s->capacity, fmt, args);
    va_end(args);
    if (new_size * 2 >= s->capacity) {
        s->capacity = new_size * 2;
        s->data = (char *)realloc(s->data, new_size * sizeof(char));
    }
    if (new_size > s->capacity) {
        snprintf(s->data, s->capacity, fmt, args);
    }
    s->size = new_size;
}

void string_free(string *s) {
    free(s->data);
    s->capacity = 0;
    s->size = 0;
}
