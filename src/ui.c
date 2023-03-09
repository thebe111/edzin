#include "include/ui.h"
#include "include/ascii.h"
#include "include/buffer.h"
#include "include/escseq.h"
#include "include/smartbuf.h"
#include <string.h>
#include <unistd.h>

void
edzin_draw_statusbar(edzin_config_t* E, edzin_smartbuf_t* buf) {
    edzin_sbuf_append(buf, ESCSEQ_REVERT_COLORS, 4);

    char* f = NULL;

    for (int i = 0; i < E->win->nbuf; i++) {
        if (E->win->buf->fname != NULL) {
            f = E->win->buf->fname;
        }
    }

    if (f == NULL) {
        f = "[No Name]";
    }

    char status[80], rstatus[80];
    char* rfiletype = (E->win->buf->syntax) ? E->win->buf->syntax->filetype : "unk";
    char* rstate = (E->win->buf->fname != NULL) ? edzin_file_state_to_str(E->win->buf->state) : "new";

    int len = snprintf(status, sizeof(status), "%.20s [%s]", f, rstate);
    int file_percent = (E->win->cursor.y * 100) / (E->win->buf->nlines > 0 ? E->win->buf->nlines : 1);
    int rfile_percent = file_percent > 100 ? 100 : file_percent;
    int rlen = snprintf(rstatus,
                        sizeof(rstatus),
                        "[%s] %d:%d\x20%d%%",
                        rfiletype,
                        E->win->cursor.y + 1,
                        E->win->cursor.x + 1,
                        rfile_percent);

    if (len > E->scr_props.cols) {
        len = E->scr_props.cols;
    }

    edzin_sbuf_append(buf, status, len);

    while (len < E->scr_props.cols) {
        if (E->scr_props.cols - len == rlen) {
            edzin_sbuf_append(buf, rstatus, rlen);
            break;
        } else {
            edzin_sbuf_append(buf, "\x20", 1);
            len++;
        }
    }

    edzin_sbuf_append(buf, "\x1b[m", 3);
    edzin_sbuf_append(buf, BREAK_LINE, 2);
}

void
edzin_draw_msgbar(edzin_config_t* E, edzin_smartbuf_t* buf) {
    edzin_sbuf_append(buf, ESCSEQ_CLEAR_LINE, 3);

    int msglen = strlen(E->status.msg);

    if (msglen > E->scr_props.cols) {
        msglen = E->scr_props.cols;
    }

    if (msglen && time(NULL) - E->status.msg_time < 5) {
        edzin_sbuf_append(buf, E->status.msg, msglen);
    }
}

char*
edzin_file_state_to_str(edzin_buf_state_t s) {
    switch (s) {
        case UNMODIFIED:
            return "-";
        case MODIFIED:
            return "+";
        default:
            return "?";
    }
}
