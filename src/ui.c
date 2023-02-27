#include "edzin/ui.h"
#include <string.h>
#include <unistd.h>

void
edzin_draw_statusbar(edzin_config_t* E, edzin_append_buf_t* buf) {
    buf_append(buf, "\x1b[7m", 4);

    char* f = NULL;

    for (int i = 0; i < E->nfiles; i++) {
        if (E->files[i].rendering == true) {
            f = E->files[i].filename;
        }
    }

    if (f == NULL) {
        f = "[No Name]";
    }

    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s [%s]", f, edzin_file_state_to_str(E->files[0].state));
    int file_percent = (E->cursor.y * 100) / (E->nlines > 0 ? E->nlines : 1);
    int rfile_percent = file_percent > 100 ? 100 : file_percent;
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d:%d\x20%d%%", E->cursor.y + 1, E->cursor.x + 1, rfile_percent);

    if (len > E->screen_props.cols) {
        len = E->screen_props.cols;
    }

    buf_append(buf, status, len);

    while (len < E->screen_props.cols) {
        if (E->screen_props.cols - len == rlen) {
            buf_append(buf, rstatus, rlen);
            break;
        } else {
            buf_append(buf, "\x20", 1);
            len++;
        }
    }

    buf_append(buf, "\x1b[m", 3);
    buf_append(buf, "\r\n", 2);
}

void
edzin_draw_msgbar(edzin_config_t* E, edzin_append_buf_t* buf) {
    buf_append(buf, "\x1b[K", 3);

    int msglen = strlen(E->status.msg);

    if (msglen > E->screen_props.cols) {
        msglen = E->screen_props.cols;
    }

    if (msglen && time(NULL) - E->status.msg_time < 5) {
        buf_append(buf, E->status.msg, msglen);
    }
}

char*
edzin_file_state_to_str(enum edzin_file_state s) {
    switch (s) {
        case UNMODIFIED:
            return "-";
        case MODIFIED:
            return "+";
        default:
            return "?";
    }
}
