#ifndef EDZIN_MAIN_H
#define EDZIN_MAIN_H

#include "highlight.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>

#define FAILURE (-1)
#define ESCAPE '\x1b'
#define TAB_STOP_SIZE 4
#define UNUSED(x) (void)(x)
#define EDZIN_VERSION "0.0.1"
#define CTRL_KEY(key) ((key) &0x1f)
#define APPEND_BUF_INIT \
    { NULL, 0 }

enum edzin_key_e {
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
    int idx;
    int size;
    int rsize;  // render size
    char* chars;
    char* render;
    unsigned char* highlight; // highlight char control buffer
    bool in_comment; // flag that control if the current line is part of a comment
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

enum edzin_file_state_e { MODIFIED, UNMODIFIED };

typedef struct {
    bool rendering;
    enum edzin_file_state_e state;
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
    edzin_syntax_t* syntax;
} edzin_config_t;

typedef struct {
    char* buf;
    int len;
} edzin_append_buf_t;

enum edzin_highlight_e {
    HL_COMMENT,
    HL_KEYTYPE,
    HL_KEYWORD,
    HL_MATCH,
    HL_MLCOMMENT,
    HL_NORMAL,
    HL_NUMBER,
    HL_STRING,
};

enum edzin_highlight_color_e {
    HL_RED = 31,
    HL_GREEN = 32,
    HL_YELLOW = 33,
    HL_BLUE = 34,
    HL_MAGENTA = 35,
    HL_CYAN = 36,
    HL_WHITE = 37,
};

char* edzin_lines_to_str(int* buflen);
char* edzin_prompt(char* prompt, void (*cb)(char* query, int c));
char* edzin_render_cursor_on_pos(int line, int col);
int edzin_get_cursor_pos();
int edzin_get_winsize();
int edzin_read_key();
int edzin_syntax_to_color(int highlight);
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
void edzin_find_callback(char* query, int c);
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
void edzin_refresh_screen(bool block_cursor);
void edzin_save();
void edzin_scroll();
void edzin_select_syntax_highlight();
void edzin_set_status_msg(const char* fmt, ...);
void edzin_update_line(edzin_line_t* line);
void edzin_update_syntax(edzin_line_t* line);

#endif  // EDZIN_MAIN_H
