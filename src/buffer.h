#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

typedef struct {
    char *data;
    unsigned raw_size, capacity;
} buffer;

buffer mk_buffer(unsigned capacity);

buffer mk_string(unsigned capacity);

void buffer_realloc(buffer *buf, unsigned capacity);

void string_clear(buffer *buf);

void buffer_putchar(buffer *buf, char c);

void buffer_append(buffer *buf, const char *data, unsigned len);

int string_nprintf(buffer *buf, unsigned maxlen, const char *fmt, ...);

void buffer_compact(buffer *buf);

void buffer_free(buffer *buf);
