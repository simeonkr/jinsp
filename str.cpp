#include <stdlib.h>
#include <string.h>
#include "str.h"

string mk_string() {
    string res;
    res.size = 1;
    res.capacity = 16;
    res.data = (char *)malloc(res.capacity * sizeof(char));
    res.data[0] = '\0';
    return res;
}

static string string_expand(string s, unsigned increase) {
    string res = s;
    res.size += increase;
    if (res.size * 2 >= res.capacity) {
        res.capacity = res.size * 2;
    }
    res.data = (char *)realloc(res.data, res.size * sizeof(char));
    return res;
}

void string_append(string s, const char *cstr) {
    string res = string_expand(s, strlen(cstr));
    strncpy(&res.data[res.size], cstr, strlen(cstr));
};

void string_printf(string s, const char *fmt, ...) {

}

void string_free(string s) {
    free(s.data);
}
