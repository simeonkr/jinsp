#include <assert.h>
#include "json.h"

#define STACK_SIZE 128

typedef struct {
    json_value value;
    int index;
} json_pos;

typedef struct {
    json_pos data[STACK_SIZE];
    int size;
} json_stack;

static inline json_pos *stack_peek(json_stack *stack) {
    return &stack->data[stack->size - 1];
}

static inline json_pos *stack_peekn(json_stack *stack, int n) {
    assert(n < stack->size);
    return &stack->data[stack->size - 1 - n];
}

static inline json_pos stack_pop(json_stack *stack) {
    return stack->data[stack->size--];
}

static inline void stack_push(json_stack *stack, json_pos pos) {
    assert(stack->size < STACK_SIZE);
    stack->data[stack->size++] = pos;
}
