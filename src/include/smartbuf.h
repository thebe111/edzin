#ifndef EDZIN_SMARTBUF_H
#define EDZIN_SMARTBUF_H

#define SMART_BUF_INIT \
    { NULL, 0 }

// buffer struct with "auto" allocation
typedef struct {
    char* buf;
    int len;
} edzin_smartbuf_t;

void edzin_draw_lines(edzin_smartbuf_t* buf);
void edzin_sbuf_append(edzin_smartbuf_t* buf, const char* s, int len);
void edzin_sbuf_free(edzin_smartbuf_t* buf);

#endif  // EDZIN_SMARTBUF_H
