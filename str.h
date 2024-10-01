#pragma once

typedef struct string {
    unsigned size, capacity;
    char *data;
} string;

string mk_empty_string();

string mk_string(const char *cstr);

void string_append(string s, const char *cstr);

void string_printf(string s, const char *fmt, ...);

void string_free(string s);
