#ifndef EDZIN_MAIN_H
#define EDZIN_MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>

#define EDZIN_VERSION "0.0.1"
#define TAB_STOP_SIZE 4

#define CTRL_KEY(key) ((key) &0x1f)
#define ESCAPE '\x1b'

#define FAILURE (-1)

#define APPEND_BUF_INIT \
    { NULL, 0 }

enum edzin_key {
    BACKSPACE = 127,
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
    int rsize;  // render size
    char* chars;
    char* render;
} edzin_line_t;

typedef struct {
    int lines;
    int cols;
} edzin_screen_props_t;

typedef struct {
    int x;
    int y;
    int rx;  // render x-axis
} edzin_cursor_t;

typedef struct {
    int x_offset;
    int y_offset;
} edzin_scroll_t;

enum edzin_file_state { MODIFIED, UNMODIFIED };

typedef struct {
    bool rendering;
    enum edzin_file_state state;
    char* filename;
} edzin_file_t;

typedef struct {
    char msg[80];
    time_t msg_time;
} edzin_status_t;

typedef struct {
    struct termios TERM_MODE;
    int nfiles;
    int nlines;
    edzin_cursor_t cursor;
    edzin_scroll_t scroll;
    edzin_screen_props_t screen_props;
    edzin_line_t* line;
    edzin_file_t* files;
    edzin_status_t status;
} edzin_config_t;

typedef struct {
    char* buf;
    int len;
} edzin_append_buf_t;

char* edzin_lines_to_str(int* buflen);
char* edzin_prompt(char* prompt);
int edzin_get_cursor_pos();
int edzin_get_winsize();
int edzin_read_key();
int edzin_transform_rx_to_x(edzin_line_t* line, int chars_rx);
int edzin_transform_x_to_rx(edzin_line_t* line, int chars_x);
void edzin_backspace_char();
void edzin_buf_append(edzin_append_buf_t* buf, const char* s, int len);
void edzin_buf_free(edzin_append_buf_t* buf);
void edzin_clean_up();
void edzin_delete_char();
void edzin_delete_line(int at);
void edzin_die(const char* msg);
void edzin_draw_lines(edzin_append_buf_t* buf);
void edzin_enable_raw_mode();
void edzin_find();
void edzin_free_line(edzin_line_t* line);
void edzin_init();
void edzin_insert_char(int c);
void edzin_insert_line(int at, char* s, size_t len);
void edzin_insert_new_line();
void edzin_line_append_str(edzin_line_t* line, char* s, size_t len);
void edzin_line_delete_char(edzin_line_t* line, int at);
void edzin_line_insert_char(edzin_line_t* line, int at, int c);
void edzin_mv_cursor(int key);
void edzin_open(char* filename);
void edzin_process_keypress();
void edzin_refresh_screen();
void edzin_save();
void edzin_scroll();
void edzin_set_status_msg(const char* fmt, ...);
void edzin_update_line(edzin_line_t* line);

#endif  // EDZIN_MAIN_H
