#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char *data;
    unsigned size, capacity;
} buffer;

buffer mk_buffer();

void buffer_append(buffer *buf, const char *data, int len);

void buffer_compact(buffer *buf);

void buffer_free(buffer buf);
