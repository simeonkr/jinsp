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

json_value mk_object_value(json_object object) {
    return { json_value::OBJECT, { .object = object } };
}

json_value mk_array_value(json_array array) {
    return { json_value::ARRAY, { .array = array } };
}

json_value mk_string_value(char *string) {
    return { json_value::STRING, { .string = string } };
}

json_value mk_number_value(float number) {
    return { json_value::NUMBER, { .number = number } };
}

json_value mk_true_value() {
    return { json_value::TRUE };
}

json_value mk_false_value() {
    return { json_value::FALSE };
}

json_value mk_null_value() {
    return { json_value::NUL };
}

// TODO: use a doubly-linked list to make this constant time
// TODO: use linux-style linked lists (list_head)
void object_append(json_object *object, char *key, json_value val) {
    assert(object != NULL);
    json_member **last;
    for (last = object; *last != NULL; last = &(*last)->next);
    *last = (json_member *)malloc(sizeof(json_member));
    (*last)->key = key;
    (*last)->val = val;
    (*last)->next = NULL;
}

json_object mk_object() {
    return NULL;
}

json_array mk_array() {
    json_array array;
    array.size = 0;
    array.capacity = 8;
    array.data = (json_value *)malloc(array.capacity * sizeof(json_value));
    return array;
};

void array_append(json_array *array, json_value val) {
    assert(array != NULL);
    array->data[array->size++] = val;
    if (array->size * 2 >= array->capacity) {
        array->capacity *= 2;
        array->data = (json_value *)realloc(
            array->data, array->capacity * sizeof(json_value));
    }
}

void array_compact(json_array *array) {
    assert(array != NULL);
    array->capacity = array->size;
    array->data = (json_value *)realloc(
        array->data, array->capacity * sizeof(json_value));
}
