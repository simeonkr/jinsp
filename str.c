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

static void string_vprintfa(string *s, const char *fmt, va_list args) {
    va_list saved_args;
    va_copy(saved_args, args);
    int new_size = s->size + 
        vsnprintf(&s->data[s->size], s->capacity - s->size, fmt, args);
    int retry = new_size + 1 > s->capacity;
    if ((new_size + 1) * 2 >= s->capacity) {
        while ((new_size + 1) * 2 >= s->capacity)
            s->capacity *= 2;
        s->data = (char *)realloc(s->data, s->capacity * sizeof(char));
    }
    if (retry) {
        vsnprintf(&s->data[s->size], s->capacity - s->size, fmt, saved_args);
    }
    s->size = new_size;
}

string mk_string(const char *fmt, ...) {
    string s = mk_empty_string();
    va_list args;
    va_start(args, fmt);
    string_vprintfa(&s, fmt, args);
    va_end(args);
    return s;
}

void string_printf(string *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    s->size = 0;
    string_vprintfa(s, fmt, args);
    va_end(args);
}

void string_printfa(string *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    string_vprintfa(s, fmt, args);
    va_end(args);
}

void string_free(string *s) {
    free(s->data);
    s->capacity = 0;
    s->size = 0;
}
