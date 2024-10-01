#include <stdio.h>
#include "json.h"
#include "print.h"

static void print_indent(int count) {
    for (int i = 0; i < count; i++)
        printf("    ");
}

static void print_top(json_value);
static void print_value(json_value, int);
static void print_object(json_object, int);
static void print_members(json_object, int);
static void print_member(json_member, int);
static void print_array(json_array, int);
static void print_elements(json_array, int);
static void print_element(json_value, int);
static void print_string(char *);
static void print_number(float);
static void print_true();
static void print_false();
static void print_null();

static void print_top(json_value value) {
    print_value(value, 0);
    putchar('\n');
}

static void print_value(json_value value, int indent) {
    switch (value.kind) {
        case OBJECT:
            print_object(value.object, indent);
            break;
        case ARRAY:
            print_array(value.array, indent);
            break;
        case STRING:
            print_string(value.string);
            break;
        case NUMBER:
            print_number(value.number);
            break;
        case TRUE:
            print_true();
            break;
        case FALSE:
            print_false();
            break;
        case NUL:
            print_null();
    }    
}

static void print_object(json_object object, int indent) {
    putchar('{'); 
    if (object_size(object) > 0) {
        putchar('\n');
        print_members(object, indent + 1);
        print_indent(indent);
    }
    putchar('}');
}

static void print_members(json_object object, int indent) {
    for (int i = 0; i < object_size(object); i++) {
        print_member(object_get(object, i), indent);
        if (i < object_size(object) - 1)
            putchar(','); 
        putchar('\n');
    }
}

static void print_member(json_member member, int indent) {
    print_indent(indent);
    print_string(member.key);
    printf(": ");
    print_value(member.val, indent);
}

static void print_array(json_array array, int indent) {
    putchar('['); 
    if (array_size(array) > 0) {
        putchar('\n');
        print_elements(array, indent + 1);
        print_indent(indent);
    }
    putchar(']');
}

static void print_elements(json_array array, int indent) {
    for (int i = 0; i < array_size(array); i++) {
        print_element(array_get(array, i), indent);
        if (i < array_size(array) - 1)
            putchar(','); 
        putchar('\n');
    }
}

static void print_element(json_value value, int indent) {
    print_indent(indent);
    print_value(value, indent);
}

static void print_string(char *s) {
    putchar('\"');
    printf("%s", s);
    putchar('\"');
}

static void print_number(float x) {
    printf("%.2f", x);
}

static void print_true() {
    printf("true");
}

static void print_false() {
    printf("false");
}

static void print_null() {
    printf("null");
}

void print_json(json_value value) {
    print_top(value);
}
