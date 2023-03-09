#include "include/window.h"
#include "include/ascii.h"
#include "include/escseq.h"
#include "include/handlers.h"
#include "include/keyboard.h"
#include "include/main.h"
#include <stdlib.h>
#include <unistd.h>

char*
edzin_render_cursor_on_pos(int line, int col) {
    const int BUF_SIZE = 32;
    char* init_buf = malloc(sizeof(char) * BUF_SIZE);

    snprintf(init_buf, BUF_SIZE, ESCSEQ_MOVE_CURSOR, line, col);

    return init_buf;
}

edzin_window_t*
new_window() {
    edzin_window_t* win = malloc(sizeof(edzin_window_t));

    *win = (edzin_window_t) {
        .buf = NULL,
        .cursor = (edzin_cursor_t) {.x = 0, .y = 0, .rx = 0},
        .nbuf = 0,
        .scroll = (edzin_scroll_t) {.x_offset = 0, .y_offset = 0},
    };

    return win;
}

int
edzin_get_cursor_pos(int* lines, int* cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return FAILURE;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }

        if (buf[i] == 'R') {
            break;
        }

        i++;
    }

    buf[i] = '\0';

    if (buf[0] != ASCII_CHAR_ESCAPE || buf[1] != '[') {
        return FAILURE;
    }

    if (sscanf(&buf[2], "%d;%d", lines, cols) != 2) {
        return FAILURE;
    }

    return EXIT_SUCCESS;
}

void
edzin_mv_cursor(int ch) {
    edzin_line_t* line = (E.win->cursor.y >= E.win->buf->nlines) ? NULL : &E.win->buf->line[E.win->cursor.y];

    switch (ch) {
        case 'h':
        case ARROW_LEFT:
            handle_mv_cursor_left(&E);
            break;
        case 'j':
        case ARROW_DOWN:
            handle_mv_cursor_down(&E);
            break;
        case 'k':
        case ARROW_UP:
            handle_mv_cursor_up(&E);
            break;
        case 'l':
        case ARROW_RIGHT:
            handle_mv_cursor_right(&E, line);
            break;
    }

    line = (E.win->cursor.y >= E.win->buf->nlines) ? NULL : &E.win->buf->line[E.win->cursor.y];
    int linelen = line ? line->size : 0;

    if (E.win->cursor.x > linelen) {
        E.win->cursor.x = linelen;
    }
}
