#pragma once

typedef struct string {
    unsigned size, capacity;
    char *data;
} string;

string mk_string();

void string_append(string s, char c);

void string_printf(string s, const char *fmt, ...);

void string_free(string s);
