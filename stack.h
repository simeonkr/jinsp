#include <assert.h>
#include "json.h"
#include "trace.h"

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
    assert(n <= stack->size);
    return &stack->data[stack->size - 1 - n];
}

static inline json_pos stack_pop(json_stack *stack) {
    return stack->data[stack->size--];
}

static inline void stack_push(json_stack *stack, json_pos pos) {
    assert(stack->size < STACK_SIZE);
    stack->data[stack->size++] = pos;
}

void trace_stack(json_stack *stack) {
    for (int i = 0; i < stack->size; i++) {
        TRACE("%d, ", stack->data[i].index);
    }
    TRACE("\n");
}

static int match(const char *haystack, const char *needle) {
    TRACE("match: %s %s = %d\n", haystack, needle, strstr(haystack, needle) != NULL);
    return strstr(haystack, needle) != NULL;
}

static void traverse_next(json_stack *stack) {
    stack_pop(stack);
    if (stack->size > 0)
        stack_peek(stack)->index++;
}

// search for str starting from (but not including) position on top of stack, 
// until either a match has been found or all contents have been popped
void search(json_stack *stack, const char *str) {
    for (int i = 0; stack->size > 0; i++) {
        trace_stack(stack);
        json_pos *top = stack_peek(stack);
        json_value val = top->value;
        switch (val.kind) {
            case OBJECT:
                if (top->index >= object_size(val.object))
                    traverse_next(stack);
                else {
                    json_member next = object_get(val.object, top->index);
                    stack_push(stack, (json_pos){ next.val, 0 });
                    if (i > 0 && match(next.key, str))
                        return;
                }
                break;
            case ARRAY:
                if (top->index >= array_size(val.array))
                    traverse_next(stack);
                else {
                    json_value next = array_get(val.array, top->index);
                    stack_push(stack, (json_pos){ next, 0 });
                }
                break;
            case STRING:
                if (i > 0 && match(val.string, str))
                    return;
                traverse_next(stack);
                break;
            case NUMBER: {
                char s[32];
                snprintf(s, 32, "%f", val.number);
                if (i > 0 && match(s, str))
                    return;
                traverse_next(stack);
                break;
            }
            case TRUE:
                if (i > 0 && match("true", str))
                    return;
                traverse_next(stack);
                break;
            case FALSE:
                if (i > 0 && match("false", str))
                    return;
                traverse_next(stack);
                break;
            case NUL:
                if (i > 0 && match("null", str))
                    return;
                traverse_next(stack);
                break;
        }
    }
}
