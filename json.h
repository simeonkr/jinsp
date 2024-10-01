#pragma once

#include <stdlib.h>
#include <assert.h>

typedef struct json_value json_value;
typedef struct json_member json_member;
typedef json_member *json_object;
typedef struct  {
    unsigned size, capacity;
    json_value *data;
} json_array;

struct json_value {
    enum { 
        OBJECT, 
        ARRAY, 
        STRING, 
        NUMBER, 
        TRUE, 
        FALSE, 
        NUL
    } kind;

    union {
        json_object object;
        json_array array;
        char *string;
        float number;
    } v;
};

struct json_member {
    char *key;
    json_value val; 
    json_member *next;
};

json_value mk_object_value(json_object object);

json_value mk_array_value(json_array array);

json_value mk_string_value(char *string);

json_value mk_number_value(float number);

json_value mk_true_value();

json_value mk_false_value();

json_value mk_null_value();

void object_append(json_object *object, char *key, json_value val);

json_object mk_object();

json_array mk_array();

void array_append(json_array *array, json_value val);

void array_compact(json_array *array);

// TODO: free values, objects, and arrays