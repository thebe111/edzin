#ifndef EDZIN_MAIN_H
#define EDZIN_MAIN_H

#include <termios.h>

#define EDZIN_VERSION "0.0.1"

#define CTRL_KEY(key) ((key) &0x1f)
#define ESCAPE '\x1b'

#define ERROR (-1)

#define APPEND_BUF_INIT \
    { NULL, 0 }

enum edzin_key {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
};

typedef struct {
    int rows;
    int cols;
} edzin_screen_props;

typedef struct {
    int x;
    int y;
} edzin_cursor;

typedef struct {
    struct termios TERM_MODE;
    edzin_screen_props screen_props;
    edzin_cursor cursor;
} edzin_config;

typedef struct {
    char* buf;
    int len;
} edzin_append_buf;

void disable_raw_mode();
void enable_raw_mode();

int get_cursor_pos();
int get_winsize();

void buf_append(edzin_append_buf* buf, const char* s, int len);
void buf_free(edzin_append_buf* buf);

int edzin_read_key();
void edzin_die(const char* msg);
void edzin_draw_rows(edzin_append_buf* buf);
void edzin_init();
void edzin_move_cursor(int key);
void edzin_process_keypress();
void edzin_refresh_screen();

#endif  // EDZIN_MAIN_H
