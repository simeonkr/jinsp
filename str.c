#include <stdlib.h>
#include <string.h>
#include "str.h"

string mk_empty_string() {
    string s;
    s.size = 0;
    s.capacity = 16;
    s.data = (char *)malloc(s.capacity * sizeof(char));
    s.data[0] = '\0';
    return s;
}

void string_append(string *s, const char *cstr) {
    unsigned new_size = s->size + strlen(cstr);
    if (new_size * 2 >= s->capacity) {
        s->capacity = new_size * 2;
        s->data = (char *)realloc(s->data, new_size * sizeof(char));
    }
    strncpy(&s->data[s->size], cstr, strlen(cstr) + 1);
    s->size = new_size;
};

string mk_string(const char *cstr) {
    string s = mk_empty_string();
    string_append(&s, cstr);
    return s;
}

void string_printf(string *s, const char *fmt, ...) {

}

void string_free(string *s) {
    free(s->data);
}
