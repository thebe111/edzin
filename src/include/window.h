#ifndef EDZIN_WINDOW_H
#define EDZIN_WINDOW_H

#include "buffer.h"

typedef struct {
    int x;
    int y;
    int rx;  // render x-axis
} edzin_cursor_t;

typedef struct {
    int x_offset;
    int y_offset;
} edzin_scroll_t;

typedef struct {
    int nbuf;
    edzin_buffer_t* buf;
    edzin_cursor_t cursor;
    edzin_scroll_t scroll;
} edzin_window_t;

char* edzin_render_cursor_on_pos(int line, int col);
edzin_window_t* new_window();
int edzin_get_cursor_pos();
void edzin_clean_up_window(edzin_window_t* win);
void edzin_mv_cursor(int ch);

#endif  // EDZIN_WINDOW_H
