#include "buffer.h"

buffer mk_buffer() {
    buffer buf;
    buf.raw_size = 0;
    buf.capacity = 64;
    buf.data = (char *)malloc(buf.capacity);
    return buf;
};

void buffer_append(buffer *buf, const char *data, int len) {
    assert(buf != NULL);
    int new_size = buf->raw_size + len / sizeof(char);
    if (new_size * 2 >= buf->capacity) {
        while (new_size * 2 >= buf->capacity)
            buf->capacity *= 2;
        buf->data = (char *)realloc(
            buf->data, buf->capacity * sizeof(char));
    }
    memcpy(&buf->data[buf->raw_size], data, len / sizeof(char));
    buf->raw_size = new_size;
}

void buffer_compact(buffer *buf) {
    assert(buf != NULL);
    buf->capacity = buf->raw_size;
    buf->data = (char *)realloc(
        buf->data, buf->capacity * sizeof(char));
}

void buffer_free(buffer buf) {
    free(buf.data);
}
