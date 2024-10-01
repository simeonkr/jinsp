#include <stdio.h>
#include <string.h>
#include "json.h"
#include "parse.h"
#include "print.h"
#include "str.h"
#include "stack.h"

#ifdef DEBUG
FILE *trace;
#endif

void parse_round_trip_test(FILE *f) {
    parse_result pr = parse_json(f);
    if (pr.success) {
        //print_json(pr.res);
        value_free(pr.res);
    }
    else {
        print_error(stderr, pr);
    }
}

void data_struct_test() {
    json_array array = mk_array();
    for (int i = 0; i < 32; i++)
        array_append(&array, mk_number_value((float)i));
    json_object object = mk_object();
    for (int i = 0; i < 10; i++)
        object_append(&object, 
            (json_member){"elements", mk_array_value(array)});
    print_json(mk_object_value(object));

    string s = mk_empty_string();
    assert(strncmp(s.data, "", s.size) == 0);
    string_free(&s);

    s = mk_string("Hello world!\n");
    assert(strncmp(s.data, "Hello world!\n", s.size) == 0);
    string_free(&s);
    assert(strncmp(s.data, "", s.size) == 0);

    s = mk_string("Hello world!\n");
    string_printf(&s, "Hello");
    assert(strncmp(s.data, "Hello", s.size) == 0);
    string_printfa(&s, " world!\n");
    assert(strncmp(s.data, "Hello world!\n", s.size) == 0);
    string_printfa(&s, "Bye bye world!\n");
    assert(strncmp(s.data, "Hello world!\nBye bye world!\n", s.size) == 0);
    string_printf(&s, "%s=%d\n", "two", 2);
    assert(strncmp(s.data, "two=2\n", s.size) == 0);
    string_free(&s);
    s = mk_empty_string();
    const char *long_str = "Long string...............................";
    string_printf(&s, "%s", long_str, 2);
    assert(strncmp(s.data, long_str, s.size) == 0);

    json_stack stack;
    stack.size = 0;
    stack_push(&stack, (json_pos){mk_string_value("val1"), 1});
    stack_push(&stack, (json_pos){mk_string_value("val2"), 2});
    assert(stack.size == 2);
    assert(stack_peek(&stack)->index == 2);
    assert(stack_peekn(&stack, 1)->index == 1);
    stack_pop(&stack);
    assert(stack_peek(&stack)->index == 1);
}

int main() {
#ifdef DEBUG
    trace = fopen("trace.txt", "w");
#endif

    parse_round_trip_test(stdin);
    data_struct_test();

#ifdef DEBUG
    fclose(trace);
#endif
}
