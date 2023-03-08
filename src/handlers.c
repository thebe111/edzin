#include "edzin/handlers.h"
#include "edzin/main.h"

#ifdef UO_ENABLE_ARROW_KEYS
int
handle_arrow_keys(char key) {
    switch (key) {
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
            return ESCAPE;
    }
}
#else
int
handle_arrow_keys(char key) {
    UNUSED(key);

    return ESCAPE;
}
#endif  // UO_ARROW_KEYS

void
handle_mv_cursor_down(edzin_config_t* E) {
    if (E->cursor.y < (E->nlines + E->screen_props.lines - 1)) {
        E->cursor.y++;
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
    if (E->cursor.x > 0) {
        E->cursor.x--;
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
    if (line && E->cursor.x < line->size) {
        E->cursor.x++;
    }
#endif  // UO_CONTINUE_SCROLL_ON_RIGHT
}

void
handle_mv_cursor_up(edzin_config_t* E) {
    if (E->cursor.y > 0) {
        E->cursor.y--;
    }
}

void
handle_page_down(edzin_config_t* E, int c) {
    if (c == PAGE_UP) {
        E->cursor.y = E->scroll.y_offset;
    } else if (c == PAGE_DOWN) {
        E->cursor.y = E->scroll.y_offset + E->screen_props.lines - 1;

        if (E->cursor.y > E->nlines) {
            E->cursor.y = E->nlines + E->screen_props.lines - 1;
        }
    }

    int times = E->screen_props.lines;

    while (times--) {
        edzin_mv_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
}
