#include "buffer.h"

buffer mk_buffer() {
    buffer buf;
    buf.size = 0;
    buf.capacity = 16;
    buf.data = (char *)malloc(buf.capacity);
    return buf;
};

void buffer_append(buffer *buf, const char *data, int len) {
    assert(buf != NULL);
    int new_size = buf->size + len / sizeof(char);
    if (new_size * 2 >= buf->capacity) {
        buf->capacity = new_size * 2;
        buf->data = (char *)realloc(
            buf->data, buf->capacity * sizeof(char));
    }
    memcpy(&buf->data[buf->size], data, len / sizeof(char));
    buf->size = new_size;
}

void buffer_compact(buffer *buf) {
    assert(buf != NULL);
    buf->capacity = buf->size;
    buf->data = (char *)realloc(
        buf->data, buf->capacity * sizeof(char));
}

void buffer_free(buffer buf) {
    free(buf.data);
}
