#include "edzin/ui.h"
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
    int len = snprintf(status, sizeof(status), "%.20s", f);
    int file_percent = (E->cursor.y * 100) / (E->num_lines > 0 ? E->num_lines : 1);
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
}
