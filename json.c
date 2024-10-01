#include "json.h"

void value_free(json_value value) {
    switch (value.kind) {
        case OBJECT:
            object_free(value.object);
            break;
        case ARRAY:
            array_free(value.array);
            break;
        case STRING:
            free(value.string);
            break;
        default:
            break;
    }
}

void object_free(json_object object) {
    for (int i = 0; i < object_size(object); i++) {
        json_member keyval = object_get(object, i);
        free(keyval.key);
        value_free(keyval.val);
    }
    buffer_free(&object);
}

void array_free(json_array array) {
    for (int i = 0; i < array_size(array); i++) {
        value_free(array_get(array, i));
    }
    buffer_free(&array);
}
