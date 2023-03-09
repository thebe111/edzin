#include "include/smartbuf.h"
#include <stdlib.h>
#include <string.h>

void
edzin_sbuf_append(edzin_smartbuf_t* buf, const char* s, int len) {
    char* new = realloc(buf->buf, buf->len + len);

    if (new == NULL) {
        return;
    }

    memcpy(&new[buf->len], s, len);
    buf->buf = new;
    buf->len += len;
}

void
edzin_sbuf_free(edzin_smartbuf_t* buf) {
    free(buf->buf);
}
