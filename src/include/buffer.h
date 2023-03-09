#ifndef EDZIN_BUFFER_H
#define EDZIN_BUFFER_H

#include "highlight.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum { MODIFIED, UNMODIFIED } edzin_buf_state_t;

typedef struct {
    int idx;
    int size;
    int rsize;  // render size
    char* chars;
    char* render;
    unsigned char* highlight;  // highlight char control buffer
    bool in_comment;  // flag that control if the current line is part of a comment
} edzin_line_t;

struct edzin_buffer {
    uint64_t idx;
    bool init;
    char* fname;
    struct edzin_buffer *next, *prev;
    edzin_buf_state_t state;
    edzin_line_t* line;
    edzin_syntax_t* syntax;
    int nlines;
};

typedef struct edzin_buffer edzin_buffer_t;

char* edzin_lines_to_str(int* buflen);
edzin_buffer_t* new_buffer(char* fname);
int edzin_transform_rx_to_x(edzin_line_t* line, int chars_rx);
int edzin_transform_x_to_rx(edzin_line_t* line, int chars_x);
void edzin_backspace_char();
void edzin_delete_char();
void edzin_delete_line(int at);
void edzin_free_line(edzin_line_t* line);
void edzin_insert_char(int ch);
void edzin_insert_line(int at, char* s, size_t len);
void edzin_insert_new_line();
void edzin_line_append_str(edzin_line_t* line, char* s, size_t len);
void edzin_line_delete_char(edzin_line_t* line, int at);
void edzin_line_insert_char(edzin_line_t* line, int at, int ch);
void edzin_update_line(edzin_line_t* line);
void edzin_update_syntax(edzin_line_t* line);
void edzin_select_syntax_highlight();

#endif  // EDZIN_BUFFER_H
