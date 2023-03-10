#include "include/handlers.h"
#include "include/ascii.h"
#include "include/keyboard.h"
#include "include/main.h"
#include <stdlib.h>
#include <string.h>

#ifdef UO_ENABLE_ARROW_KEYS
int
handle_arrow_keys(char ch) {
    switch (ch) {
        case 'A':
            return ARROW_UP;
        case 'B':
            return ARROW_DOWN;
        case 'C':
            return ARROW_RIGHT;
        case 'D':
            return ARROW_LEFT;
        default:
            edzin_set_status_msg("ERR: error processing UO_ARROW_KEYS");
            return ASCII_CHAR_ESCAPE;
    }
}
#else
int
handle_arrow_keys(char ch) {
    UNUSED(ch);

    return ASCII_CHAR_ESCAPE;
}
#endif  // UO_ARROW_KEYS

void
handle_goto(edzin_config_t* E) {
    int ch;
    char* num = "";

    while ((ch = edzin_read_key())) {
        if (ch == ASCII_CHAR_CARRIAGE_RET) {
            E->win->cursor.x = atoi(num);
            break;
        } else if (ch >= '0' && ch <= '9') {
            strcat(num, (const char*) &ch);
        } else if (ch == 'g') {
            E->win->cursor.y = 0;
            break;
        }

        break;
    }
}

void
handle_mv_cursor_down(edzin_config_t* E) {
    if (E->win->cursor.y < (E->win->buf->nlines + E->scr_props.lines - 1)) {
        E->win->cursor.y++;
    }
}

void
handle_mv_cursor_left(edzin_config_t* E) {
#ifdef UO_CONTINUE_SCROLL_ON_LEFT
    if (E->cursor.x != 0) {
        E->cursor.x--;
    } else if (E->cursor.y > 0) {
        E->cursor.y--;
        E->cursor.x = E->line[E->cursor.y].size;
    }
#else
    if (E->win->cursor.x > 0) {
        E->win->cursor.x--;
    }
#endif  // UO_CONTINUE_SCROLL_ON_LEFT
}

void
handle_mv_cursor_right(edzin_config_t* E, edzin_line_t* line) {
#ifdef UO_CONTINUE_SCROLL_ON_RIGHT
    if (line && E->cursor.x < line->size) {
        E->cursor.x++;
    } else if (line && E->cursor.x == line->size) {
        E->cursor.y++;
        E->cursor.x = 0;
    }
#else
    if (line && E->win->cursor.x < line->size) {
        E->win->cursor.x++;
    }
#endif  // UO_CONTINUE_SCROLL_ON_RIGHT
}

void
handle_mv_cursor_up(edzin_config_t* E) {
    if (E->win->cursor.y > 0) {
        E->win->cursor.y--;
    }
}

void
handle_page_down(edzin_config_t* E, int ch) {
    if (ch == PAGE_UP) {
        E->win->cursor.y = E->win->scroll.y_offset;
    } else if (ch == PAGE_DOWN) {
        E->win->cursor.y = E->win->scroll.y_offset + E->scr_props.lines - 1;

        if (E->win->cursor.y > E->win->buf->nlines) {
            E->win->cursor.y = E->win->buf->nlines + E->scr_props.lines - 1;
        }
    }

    int times = E->scr_props.lines;

    while (times--) {
        edzin_mv_cursor(ch == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
}
