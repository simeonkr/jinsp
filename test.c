#include <stdio.h>
#include <string.h>
#include "json.h"
#include "parse.h"
#include "print.h"
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

    buffer s = mk_string(16);
    assert(strncmp(s.data, "", s.raw_size) == 0);
    buffer_free(&s);

    s = mk_string(16);
    string_nprintf(&s, 0, "Hello world!\n");
    assert(s.raw_size == 14 && s.capacity == 32);
    assert(strncmp(s.data, "Hello world!\n", s.raw_size) == 0);
    string_clear(&s);
    assert(strncmp(s.data, "", s.raw_size) == 0);
    buffer_free(&s);

    s = mk_string(16);
    string_nprintf(&s, 0, "Hello");
    string_nprintf(&s, 0, " world");
    s.raw_size--;
    buffer_putchar(&s, '!');
    buffer_putchar(&s, '\n');
    buffer_putchar(&s, '\0');
    assert(strncmp(s.data, "Hello world!\n", s.raw_size) == 0);
    assert(s.raw_size == 14 && s.capacity == 32);
    string_nprintf(&s, 0, "Bye bye world!");
    string_nprintf(&s, 0, "%d", 10);
    assert(strncmp(s.data, "Hello world!\nBye bye world!10", s.raw_size) == 0);
    buffer_free(&s);

    s = mk_string(16);
    const char *long_str = "Long string...............................";
    string_nprintf(&s, 0, "%s", long_str);
    assert(strncmp(s.data, long_str, s.raw_size) == 0);
    buffer_free(&s);

    s = mk_string(16);
    string_nprintf(&s, 11, "%s", long_str);
    assert(strncmp(s.data, "Long strin", s.raw_size) == 0);
    buffer_free(&s);

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
