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

static void traverse_next(json_stack *stack, int rev) {
    stack_pop(stack);
    if (stack->size > 0) {
        if (!rev)
            stack_peek(stack)->index++;
        else
            stack_peek(stack)->index--;
    }
}

static int first_index(json_value val, int rev) {
    if (rev && val.kind == OBJECT)
        return object_size(val.object) - 1;
    else if (rev && val.kind == ARRAY)
        return array_size(val.array) - 1;
    else
        return 0;
}

// search for str starting from (but not including) position on top of stack, 
// until either a match has been found or all contents have been popped
void search(json_stack *stack, const char *str, int rev) {
    for (int i = 0; stack->size > 0; i++) {
        trace_stack(stack);
        json_pos *top = stack_peek(stack);
        json_value val = top->value;
        switch (val.kind) {
            case OBJECT:
                if (top->index >= object_size(val.object) || top->index < 0)
                    traverse_next(stack, rev);
                else {
                    json_member next = object_get(val.object, top->index);
                    int si = first_index(next.val, rev);
                    stack_push(stack, (json_pos){ next.val, si });
                    if (i > 0 && match(next.key, str))
                        return;
                }
                break;
            case ARRAY:
                if (top->index >= array_size(val.array) || top->index < 0)
                    traverse_next(stack, rev);
                else {
                    json_value next = array_get(val.array, top->index);
                    int si = first_index(next, rev);
                    stack_push(stack, (json_pos){ next, si });
                }
                break;
            case STRING:
                if (i > 0 && match(val.string, str))
                    return;
                traverse_next(stack, rev);
                break;
            case NUMBER: {
                char s[32];
                snprintf(s, 32, "%f", val.number);
                if (i > 0 && match(s, str))
                    return;
                traverse_next(stack, rev);
                break;
            }
            case TRUE:
                if (i > 0 && match("true", str))
                    return;
                traverse_next(stack, rev);
                break;
            case FALSE:
                if (i > 0 && match("false", str))
                    return;
                traverse_next(stack, rev);
                break;
            case NUL:
                if (i > 0 && match("null", str))
                    return;
                traverse_next(stack, rev);
                break;
        }
    }
}
