#ifndef EDZIN_MAIN_H
#define EDZIN_MAIN_H

#include "highlight.h"
#include "window.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>

#define EDZIN_VERSION "0.0.1"
#define FAILURE (-1)
#define UNUSED(x) (void) (x)

typedef enum {
    NORMAL,
    INSERT,
    VISUAL,
    COMMAND,
} edzin_mode_t;

typedef struct {
    int lines;
    int cols;
} edzin_scr_props_t;

typedef struct {
    char msg[80];
    time_t msg_time;
} edzin_status_t;

typedef struct {
    struct termios TERM_MODE;
    edzin_scr_props_t scr_props;
    edzin_status_t status;
    edzin_window_t* win;
} edzin_config_t;

extern edzin_config_t E;

char* edzin_prompt(char* prompt, void (*cb)(char* query, int ch));
int edzin_get_scrsize();
int edzin_read_key();
int edzin_syntax_to_color(int highlight);
void edzin_clean_up();
void edzin_die(const char* msg);
void edzin_enable_raw_mode();
void edzin_find();
void edzin_find_callback(char* query, int ch);
void edzin_init();
void edzin_open(char* fname);
void edzin_process_keypress();
void edzin_refresh_screen(bool block_cursor);
void edzin_save();
void edzin_scroll();
void edzin_set_status_msg(const char* fmt, ...);

#endif  // EDZIN_MAIN_H
