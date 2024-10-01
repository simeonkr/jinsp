#pragma once

#include <stdlib.h>
#include <assert.h>
#include "buffer.h"

typedef struct json_value json_value;
typedef struct json_member json_member;
typedef buffer json_object;
typedef buffer json_array;

struct json_value {
    enum { 
        NUL,
        OBJECT, 
        ARRAY, 
        STRING, 
        NUMBER, 
        TRUE, 
        FALSE
    } kind;

    union {
        json_object object;
        json_array array;
        char *string;
        float number;
    };
};

struct json_member {
    char *key;
    json_value val; 
};

static inline json_value mk_object_value(json_object object) {
    return (json_value) { OBJECT, { .object = object } };
}

static inline json_value mk_array_value(json_array array) {
    return (json_value) { ARRAY, { .array = array } };
}

static inline json_value mk_string_value(char *string) {
    return (json_value) { STRING, { .string = string } };
}

static inline json_value mk_number_value(float number) {
    return (json_value) { NUMBER, { .number = number } };
}

static inline json_value mk_true_value() {
    return (json_value) { TRUE };
}

static inline json_value mk_false_value() {
    return (json_value) { FALSE };
}

static inline json_value mk_null_value() {
    return (json_value) { NUL };
}

void value_free(json_value value);

static inline json_object mk_object() {
    return mk_buffer(64);
}

static inline unsigned object_size(json_object object) {
    return object.raw_size / sizeof(json_member);
}

static inline json_member object_get(json_object object, int index) {
    return ((json_member *)object.data)[index];
}

static inline void object_append(json_object *object, json_member keyval) {
    buffer_append(object, (const char *)&keyval, sizeof(json_member));
}

void object_free(json_object object);

static inline json_array mk_array() {
    return mk_buffer(64);
};

static inline unsigned array_size(json_array array) {
    return array.raw_size / sizeof(json_value);
}

static inline json_value array_get(json_object array, int index) {
    return ((json_value *)array.data)[index];
}

static inline void array_append(json_array *array, json_value val) {
    buffer_append(array, (const char *)&val, sizeof(json_value));
}

void array_free(json_array array);
