#include <stdio.h>
#include "buffer.h"
#include "util.h"

buffer mk_buffer(unsigned capacity) {
    assert(capacity > 0);
    buffer buf;
    buf.raw_size = 0;
    buf.capacity = capacity;
    buf.data = (char *)malloc(buf.capacity);
    return buf;
};

buffer mk_string(unsigned capacity) {
    buffer buf = mk_buffer(capacity);
    buf.raw_size = 1;
    buf.data[0] = '\0';
    return buf;
}

void buffer_realloc(buffer *buf, unsigned capacity) {
    buf->data = (char *)realloc(buf->data, capacity);
    buf->capacity = capacity;
}

void string_clear(buffer *buf) {
    assert(buf);
    buf->data[0] = '\0';
    buf->raw_size = 1;
}

static inline void buffer_request_size(buffer *buf, unsigned new_size) {
    if (new_size * 2 >= buf->capacity) {
        while (new_size * 2 >= buf->capacity)
            buf->capacity *= 2;
        buf->data = (char *)realloc(buf->data, buf->capacity);
    }
}

void buffer_putchar(buffer *buf, char c) {
    buffer_request_size(buf, buf->raw_size + 1);
    buf->data[buf->raw_size] = c;
    buf->raw_size++;
}

void buffer_append(buffer *buf, const char *data, unsigned len) {
    assert(buf);
    unsigned new_size = buf->raw_size + len;
    buffer_request_size(buf, new_size);
    memcpy(&buf->data[buf->raw_size], data, len);
    buf->raw_size = new_size;
}

// assumes that the last char is '\0' and may be overwritten
static inline unsigned string_space(buffer *buf, unsigned maxlen) {
    unsigned space = buf->capacity - buf->raw_size + 1;
    if (maxlen == 0)
        return space;
    else
        return min(space, maxlen);
}

// maxlen includes '\0'
// returns number of characters written, excluding '\0'
int string_nprintf(buffer *buf, unsigned maxlen, const char *fmt, ...) {
    assert(buf);
    assert(buf->raw_size >= 1);
    assert(buf->data[buf->raw_size - 1] == '\0');

    va_list args, saved_args;
    va_start(args, fmt);
    va_copy(saved_args, args);
    int space = string_space(buf, maxlen);
    // nw = number of characters that were or will be written, excluding '\0'
    int nw = vsnprintf(&buf->data[buf->raw_size - 1], space, fmt, args);
    if (maxlen != 0)
        nw = min(nw, maxlen - 1);
    va_end(args);

    int retry = nw + 1 > space;

    buffer_request_size(buf, buf->raw_size + nw);

    space = string_space(buf, maxlen);
    if (retry)
        vsnprintf(&buf->data[buf->raw_size - 1], space, fmt, saved_args);
    va_end(saved_args);
    buf->raw_size += nw;

    assert(buf->raw_size * 2 < buf->capacity);
    assert(buf->data[buf->raw_size - 1] == '\0');
    return nw;
}

void buffer_compact(buffer *buf) {
    assert(buf);
    buf->capacity = buf->raw_size;
    buf->data = (char *)realloc(buf->data, buf->capacity);
}

void buffer_free(buffer *buf) {
    assert(buf);
    free(buf->data);
    buf = NULL;
}
