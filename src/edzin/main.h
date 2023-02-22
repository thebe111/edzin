#ifndef EDZIN_MAIN_H
#define EDZIN_MAIN_H

#include <stdio.h>
#include <termios.h>

#define EDZIN_VERSION "0.0.1"

#define CTRL_KEY(key) ((key) &0x1f)
#define ESCAPE '\x1b'

#define FAILURE (-1)

#define APPEND_BUF_INIT \
    { NULL, 0 }

enum edzin_key {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DELETE_KEY,
};

typedef struct {
    int size;
    char* content;
} edzin_row_t;

typedef struct {
    int rows;
    int cols;
} edzin_screen_props_t;

typedef struct {
    int x;
    int y;
} edzin_cursor_t;

typedef struct {
    int x_offset;
    int y_offset;
} edzin_scroll_t;

typedef struct {
    struct termios TERM_MODE;
    edzin_screen_props_t screen_props;
    edzin_cursor_t cursor;
    int numrows;
    edzin_row_t* row;
    edzin_scroll_t scroll;
} edzin_config_t;

typedef struct {
    char* buf;
    int len;
} edzin_append_buf_t;

void disable_raw_mode();
void enable_raw_mode();

int get_cursor_pos();
int get_winsize();

void buf_append(edzin_append_buf_t* buf, const char* s, int len);
void buf_free(edzin_append_buf_t* buf);

int edzin_read_key();
void edzin_append_row(char* s, size_t len);
void edzin_die(const char* msg);
void edzin_draw_rows(edzin_append_buf_t* buf);
void edzin_init();
void edzin_move_cursor(int key);
void edzin_open(char* filename);
void edzin_process_keypress();
void edzin_refresh_screen();
void edzin_scroll();

#endif  // EDZIN_MAIN_H
