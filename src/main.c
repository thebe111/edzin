#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "edzin/main.h"
#include "edzin/ascii.h"
#include "edzin/escseq.h"
#include "edzin/handlers.h"
#include "edzin/highlight.h"
#include "edzin/lexer.h"
#include "edzin/ui.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const char HELP[] = "HELP: Ctrl+S = save | Ctrl+Q = quit | Ctrl+F = search";

/*
 * main editor config setup
 */
edzin_config_t E;

#ifndef TEST
int
main(int argc, char** argv) {
    edzin_enable_raw_mode();
    edzin_init();

    if (argc >= 2) {
        edzin_open(/*filename=*/argv[1]);
    }

    edzin_set_status_msg(HELP);

    while (true) {
        edzin_refresh_screen(false);
        edzin_process_keypress();
    }

    return EXIT_SUCCESS;
}
#endif

char*
edzin_lines_to_str(int* buflen) {
    int totallen = 0;

    for (int i = 0; i < E.nlines; i++) {
        totallen += E.line[i].size + 1;
    }

    *buflen = totallen;

    char* buf = malloc(totallen);
    char* p = buf;

    for (int i = 0; i < E.nlines; i++) {
        memcpy(p, E.line[i].chars, E.line[i].size);
        p += E.line[i].size;
        *p = '\n';
        p++;
    }

    return buf;
}

char*
edzin_prompt(char* prompt, void (*cb)(char* query, int c)) {
    size_t bufsize = 128;
    char* buf = malloc(bufsize);
    size_t buflen = 0;
    bool block_cursor = true;

    buf[0] = ASCII_NULL;

    while (true) {
        edzin_set_status_msg(prompt, buf);
        E.cursor.x = buflen + strlen(prompt) - 2;
        edzin_refresh_screen(block_cursor);

        int c = edzin_read_key();

        if (c == DELETE_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = ASCII_NULL;
            }
        } else if (c == ESCAPE) {
            block_cursor = false;
            edzin_set_status_msg("");

            if (cb) {
                cb(buf, c);
            }

            free(buf);

            return NULL;
        } else if (c == '\r') {
            block_cursor = false;

            if (buflen != 0) {
                edzin_set_status_msg("");

                if (cb) {
                    cb(buf, c);
                }

                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }

            buf[buflen++] = c;
            buf[buflen] = '\0';
        }

        if (cb) {
            cb(buf, c);
        }
    }
}

char*
edzin_render_cursor_on_pos(int line, int col) {
    const int BUF_SIZE = 32;
    char* init_buf = malloc(sizeof(char) * BUF_SIZE);

    snprintf(init_buf, BUF_SIZE, ESCSEQ_MOVE_CURSOR, line, col);

    return init_buf;
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

    if (buf[0] != ESCAPE || buf[1] != '[') {
        return FAILURE;
    }

    if (sscanf(&buf[2], "%d;%d", lines, cols) != 2) {
        return FAILURE;
    }

    return EXIT_SUCCESS;
}

int
edzin_get_winsize(int* lines, int* cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == FAILURE || ws.ws_col == 0) {
        // hard way to get all terminals screen size, moving the cursor to the bottom-right corner
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return edzin_get_cursor_pos(lines, cols);
        }

        edzin_read_key();

        return FAILURE;
    }

    *cols = ws.ws_col;
    *lines = ws.ws_row;

    return EXIT_SUCCESS;
}

int
edzin_read_key() {
    char c;
    int nread;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == FAILURE && errno != EAGAIN) {
            edzin_die("read");
        }
    }

    if (c == ESCAPE) {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1 || read(STDIN_FILENO, &seq[1], 1) != 1) {
            return ESCAPE;
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return ESCAPE;
                }

                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DELETE_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            } else {
                switch (seq[1]) {
                    case 'A':
                    case 'B':
                    case 'C':
                    case 'D':
                        return handle_arrow_keys(seq[1]);
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            }
        }

        return ESCAPE;
    }

    return c;
}

int
edzin_syntax_to_color(int highlight) {
    switch (highlight) {
        case HL_KEYTYPE:
            return HL_YELLOW;
        case HL_KEYWORD:
            return HL_GREEN;
        case HL_COMMENT:
        case HL_MLCOMMENT:
            return HL_CYAN;
        case HL_STRING:
            return HL_MAGENTA;
        case HL_NUMBER:
            return HL_RED;
        case HL_MATCH:
            return HL_BLUE;
        default:
            return HL_WHITE;
    }
}

int
edzin_transform_rx_to_x(edzin_line_t* line, int chars_rx) {
    int x;
    int cur_rx = 0;

    for (x = 0; x < line->size; x++) {
        if (line->chars[x] == ASCII_TAB) {
            cur_rx += (TAB_STOP_SIZE - 1) - (cur_rx % TAB_STOP_SIZE);
        }

        cur_rx++;

        if (cur_rx > chars_rx) {
            return x;
        }
    }

    return x;
}

int
edzin_transform_x_to_rx(edzin_line_t* line, int chars_x) {
    int render_x = 0;

    for (int i = 0; i < chars_x; i++) {
        if (line->chars[i] == ASCII_TAB) {
            render_x += (TAB_STOP_SIZE - 1) - (render_x % TAB_STOP_SIZE);
        }

        render_x++;
    }

    return render_x;
}

void
edzin_insert_line(int at, char* s, size_t len) {
    if (at < 0 || at > E.nlines) {
        return;
    }

    E.line = realloc(E.line, sizeof(edzin_line_t) * (E.nlines + 1));
    memmove(&E.line[at + 1], &E.line[at], sizeof(edzin_line_t) * (E.nlines - at));

    for (int i = at + 1; i <= E.nlines; i++) {
        E.line[i].idx++;
    }

    E.line[at].idx = at;
    E.line[at].size = len;
    E.line[at].chars = malloc(len + 1);
    memcpy(E.line[at].chars, s, len);
    E.line[at].chars[len] = ASCII_NULL;
    E.line[at].rsize = 0;
    E.line[at].render = NULL;
    E.line[at].highlight = NULL;
    E.line[at].in_comment = false;
    edzin_update_line(&E.line[at]);
    E.nlines++;
}

void
edzin_insert_new_line() {
    if (E.cursor.x == 0) {
        edzin_insert_line(E.cursor.y, "", 0);
    } else {
        edzin_line_t* line = &E.line[E.cursor.y];

        edzin_insert_line(E.cursor.y + 1, &line->chars[E.cursor.x], line->size - E.cursor.x);
        line = &E.line[E.cursor.y];
        line->size = E.cursor.x;
        line->chars[line->size] = ASCII_NULL;
        edzin_update_line(line);
    }

    E.cursor.y++;
    E.cursor.x = 0;
}

void
edzin_backspace_char() {
    edzin_line_t* line = &E.line[E.cursor.y];

    if (E.cursor.x > 0) {
        edzin_line_delete_char(line, E.cursor.x - 1);
        E.cursor.x--;
    }

#ifdef UO_ENABLE_DELETE_LINE_JOIN
    if (E.cursor.y > 0) {
        edzin_line_t* prev_line = &E.line[E.cursor.y - 1];

        E.cursor.x = prev_line->size + line->size;
        edzin_line_append_str(prev_line, line->chars, line->size);
        edzin_delete_line(E.cursor.y);
        E.cursor.y--;
    }
#endif
}

void
edzin_buf_append(edzin_append_buf_t* buf, const char* s, int len) {
    char* new = realloc(buf->buf, buf->len + len);

    if (new == NULL) {
        return;
    }

    memcpy(&new[buf->len], s, len);
    buf->buf = new;
    buf->len += len;
}

void
edzin_buf_free(edzin_append_buf_t* buf) {
    free(buf->buf);
}

void
edzin_clean_up() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.TERM_MODE) == FAILURE) {
        edzin_die("tcsetattr");
    }

    for (int i = 0; i < E.nlines; i++) {
        free(E.line[i].chars);
        free(E.line[i].render);
    }

    for (int i = 0; i < E.nfiles; i++) {
        free(E.files[i].filename);
    }

    free(E.files);
}

void
edzin_delete_char() {
    if (E.cursor.y == E.nlines) {
        return;
    }

    if (E.cursor.x == 0 && E.cursor.y == 0) {
        return;
    }

    edzin_line_t* line = &E.line[E.cursor.y];

    if (E.cursor.x > 0 && E.cursor.x < *(&E.line[E.cursor.y].rsize)) {
        edzin_mv_cursor(ARROW_RIGHT);
        edzin_line_delete_char(line, E.cursor.x - 1);
        E.cursor.x--;
    }

#ifdef UO_ENABLE_DELETE_LINE_JOIN
    if (E.cursor.y + 1 < E.nlines) {
        edzin_line_t* next_line = &E.line[E.cursor.y + 1];

        E.cursor.x = line->size + next_line->size;
        edzin_line_append_str(line, next_line->chars, next_line->size);
        edzin_delete_line(E.cursor.y + 1);
    }
#endif
}

void
edzin_delete_line(int at) {
    if (at < 0 || at >= E.nlines) {
        return;
    }

    edzin_free_line(&E.line[at]);
    memmove(&E.line[at], &E.line[at - 1], sizeof(edzin_line_t) * (E.nlines - at - 1));

    for (int i = at; i < E.nlines - 1; i++) {
        E.line[i].idx--;
    }

    E.nlines--;

    if (E.files != NULL) {
        E.files[0].state = MODIFIED;
    }
}

void
edzin_die(const char* msg) {
    write(STDOUT_FILENO, ESCSEQ_CLEAR_SCREEN, 4);
    write(STDOUT_FILENO, ESCSEQ_RESET_CURSOR, 3);
    perror(msg);
    exit(EXIT_FAILURE);
}

void
edzin_draw_lines(edzin_append_buf_t* buf) {
    for (int i = 0; i < E.screen_props.lines; i++) {
        int file_line = i + E.scroll.y_offset;

        if (file_line >= E.nlines) {
            bool display_greatings = E.nlines == 0;

            if (display_greatings && i == E.screen_props.lines / 3) {
                char greatings_msg[80];
                int greatings_len = snprintf(greatings_msg, sizeof(greatings_msg), "Edzin :: v%s\r\n", EDZIN_VERSION);

                if (greatings_len > E.screen_props.cols) {
                    greatings_len = E.screen_props.cols;
                }

                int padding = (E.screen_props.cols - greatings_len) / 2;

                if (padding) {
                    edzin_buf_append(buf, "~", 1);
                    padding--;
                }

                while (padding--) {
                    edzin_buf_append(buf, " ", 1);
                }

                edzin_buf_append(buf, greatings_msg, greatings_len);
            } else {
                edzin_buf_append(buf, "~", 1);
            }
        } else {
            int len = E.line[file_line].rsize - E.scroll.x_offset;

            if (len < 0) {
                len = 0;
            }

            if (len > E.screen_props.cols) {
                len = E.screen_props.cols;
            }

            char* c = &E.line[file_line].render[E.scroll.x_offset];
            unsigned char* highlight = &E.line[file_line].highlight[E.scroll.x_offset];

            for (int i = 0; i < len; i++) {
#if 0 // @@@
                if (iscntrl(c[i])) {
                    char sym = (c[i] <= 26) ? '@' + c[i] : '?';

                    edzin_buf_append(buf, ESCSEQ_REVERT_COLORS, 4);
                    edzin_buf_append(buf, &sym, 1);
                    edzin_buf_append(buf, "\x1b[m", 3);
                } 
#endif

                if (highlight[i] == HL_NORMAL) {
                    edzin_buf_append(buf, ESCSEQ_COLOR_DEF, 5);
                    edzin_buf_append(buf, &c[i], 1);
                } else {
                    static int COLOR_BUFLEN = 16;
                    char color_buf[COLOR_BUFLEN];
                    int color = edzin_syntax_to_color(highlight[i]);
                    int colorlen = snprintf(color_buf, COLOR_BUFLEN, ESCSEQ_COLOR, color);

                    edzin_buf_append(buf, color_buf, colorlen);
                    edzin_buf_append(buf, &c[i], 1);
                }
            }

            edzin_buf_append(buf, ESCSEQ_COLOR_DEF, 5);
        }

        edzin_buf_append(buf, ESCSEQ_CLEAR_LINE, 3);
        edzin_buf_append(buf, ESCSEQ_BREAK_LINE, 2);
    }
}

void
edzin_enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &E.TERM_MODE) == FAILURE) {
        edzin_die("tcgetattr");
    }

    atexit(edzin_clean_up);
    struct termios term_raw_mode = E.TERM_MODE;
    term_raw_mode.c_cflag |= (/*set chars size (CS) to 8 bits per byte=*/CS8);
    term_raw_mode.c_oflag = ~(/*turn off output processing (\n|\r\n)=*/OPOST);
    term_raw_mode.c_iflag = ~(
        /*disable ctrl+(s|q)=*/IXON |
        /*fix ctrl+m=*/ICRNL |
        /*disable breaking signal e.g: ctrl+c=*/BRKINT |
        /*enable parity checking=*/INPCK |
        /*8th bith will be stripped, flipped to 0=*/ISTRIP);
    term_raw_mode.c_lflag = ~(
        /*disable echoing=*/ECHO |
        /*disable canonical mode=*/ICANON |
        /*disable ctrl+(c|z)=*/ISIG |
        /*disable ctrl+v=*/IEXTEN);
    term_raw_mode.c_cc[VMIN] = 0;
    term_raw_mode.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_raw_mode) == FAILURE) {
        edzin_die("tcsetattr");
    }
}

void
edzin_find() {
    edzin_cursor_t bkp_cursor = {
        .x = E.cursor.x,
        .y = E.cursor.y,
    };

    edzin_scroll_t bkp_scroll = {
        .x_offset = E.scroll.x_offset,
        .y_offset = E.scroll.y_offset,
    };

    char* query = edzin_prompt("query: %s", edzin_find_callback);

    if (query) {
        free(query);

        return;
    }

    E.cursor.x = bkp_cursor.x;
    E.cursor.y = bkp_cursor.y;
    E.scroll.x_offset = bkp_scroll.x_offset;
    E.scroll.y_offset = bkp_scroll.y_offset;
}

void
edzin_find_callback(char* query, int c) {
    static int prev_highlight_line;
    static char* prev_highlight = NULL;

    if (prev_highlight) {
        memcpy(E.line[prev_highlight_line].highlight, prev_highlight, E.line[prev_highlight_line].rsize);
        free(prev_highlight);
        prev_highlight = NULL;
    }

    if (c == ESCAPE) {
        return;
    }

    for (int i = 0; i < E.nlines; i++) {
        edzin_line_t* line = &E.line[i];
        char* match = strstr(line->render, query);

        if (match) {
            E.cursor.y = i;
            E.cursor.x = edzin_transform_rx_to_x(line, match - line->render);
            E.scroll.y_offset = E.nlines;
            prev_highlight_line = i;
            prev_highlight = malloc(line->rsize);
            memcpy(prev_highlight, line->highlight, line->rsize);
            memset(&line->highlight[match - line->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void
edzin_free_line(edzin_line_t* line) {
    free(line->render);
    free(line->chars);
    free(line->highlight);
}

void
edzin_init() {
    E.nlines = 0;
    E.nfiles = 0;
    E.line = NULL;
    E.files = NULL;
    E.syntax = NULL;
    E.cursor = (edzin_cursor_t) {
        .x = 0,
        .y = 0,
        .rx = 0,
    };
    E.scroll = (edzin_scroll_t) {
        .x_offset = 0,
        .y_offset = 0,
    };
    E.status = (edzin_status_t) {
        .msg = {'\0'},
        .msg_time = 0,
    };

    if (edzin_get_winsize(&E.screen_props.lines, &E.screen_props.cols) == FAILURE) {
        edzin_die("get_winsize");
    }

    E.screen_props.lines -= 2;
}

void
edzin_insert_char(int c) {
    if (E.cursor.y == E.nlines) {
        edzin_insert_line(E.nlines, "", 0);
    }

    edzin_line_insert_char(&E.line[E.cursor.y], E.cursor.x, c);
    E.cursor.x++;

    if (E.files != NULL) {
        E.files[0].state = MODIFIED;
    }
}

void
edzin_line_append_str(edzin_line_t* line, char* s, size_t len) {
    line->chars = realloc(line->chars, line->size + len + 1);
    memcpy(&line->chars[line->size], s, len);
    line->size += len;
    line->chars[line->size] = ASCII_NULL;
    edzin_update_line(line);

    if (E.files != NULL) {
        E.files[0].state = MODIFIED;
    }
}

void
edzin_line_delete_char(edzin_line_t* line, int at) {
    if (at < 0 && at >= line->size) {
        return;
    }

    memmove(&line->chars[at], &line->chars[at + 1], line->size - at);
    line->size--;
    edzin_update_line(line);

    if (E.files != NULL) {
        E.files[0].state = MODIFIED;
    }
}

void
edzin_line_insert_char(edzin_line_t* line, int at, int c) {
    if (at < 0 || at > line->size) {
        at = line->size;
    }

    line->chars = realloc(line->chars, line->size + 2);
    memmove(&line->chars[at + 1], &line->chars[at], line->size - at + 1);
    line->size++;
    line->chars[at] = c;
    edzin_update_line(line);
}

void
edzin_mv_cursor(int key) {
    edzin_line_t* line = (E.cursor.y >= E.nlines) ? NULL : &E.line[E.cursor.y];

    switch (key) {
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

    line = (E.cursor.y >= E.nlines) ? NULL : &E.line[E.cursor.y];
    int linelen = line ? line->size : 0;

    if (E.cursor.x > linelen) {
        E.cursor.x = linelen;
    }
}

void
edzin_open(char* filename) {
    edzin_file_t file = {
        .rendering = true,
        .state = UNMODIFIED,
        .filename = strdup(filename),
    };

    E.nfiles = 1;
    E.files = calloc(1, sizeof(edzin_file_t));
    *E.files = file;

    edzin_select_syntax_highlight();

    FILE* f = fopen(filename, "r");

    if (!f) {
        edzin_die("fopen");
    }

    ssize_t linelen;
    char* line = NULL;
    size_t line_capacity = 0;

    while ((linelen = getline(&line, &line_capacity, f)) != FAILURE) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            linelen--;
        }

        edzin_insert_line(E.nlines, line, linelen);
    }

    free(line);
    fclose(f);
}

void
edzin_process_keypress() {
    int c = edzin_read_key();

    switch (c) {
        case '\r':
            edzin_insert_new_line();
            break;
        case CTRL_KEY('q'):
            /* if (E.files[0].state == MODIFIED) { */
            /* edzin_set_status_msg("WARN: file has unsaved changes"); */
            /* return; */
            /* } */
            write(STDOUT_FILENO, ESCSEQ_CLEAR_SCREEN, 4);
            write(STDOUT_FILENO, ESCSEQ_RESET_CURSOR, 3);
            exit(EXIT_SUCCESS);
            break;
        case CTRL_KEY('s'):
            edzin_save();
            break;
        case CTRL_KEY('f'):
            edzin_find();
            break;
        case 'G':
            E.cursor.y = E.nlines;
            break;
        case HOME_KEY:
            E.cursor.x = 0;
            break;
        case END_KEY: {
            if (E.cursor.y < E.nlines) {
                E.cursor.x = E.line[E.cursor.y].size;
            }
        } break;
        case BACKSPACE:
            edzin_backspace_char();
            break;
        case CTRL_KEY('h'):
        case DELETE_KEY:
            edzin_delete_char();
            break;
        case PAGE_UP:
        case PAGE_DOWN:
            handle_page_down(&E, c);
            break;
        case 'h':
        case ARROW_LEFT:
        case 'j':
        case ARROW_DOWN:
        case 'k':
        case ARROW_UP:
        case 'l':
        case ARROW_RIGHT:
            edzin_mv_cursor(c);
            break;
        case CTRL_KEY('l'):
        case ESCAPE:
            break;
        default:
            edzin_insert_char(c);
            break;
    }
}

void
edzin_refresh_screen(bool block_cursor) {
    edzin_scroll();

    edzin_append_buf_t buf = APPEND_BUF_INIT;

    edzin_buf_append(&buf, ESCSEQ_HIDE_CURSOR, 6);
    edzin_buf_append(&buf, ESCSEQ_RESET_CURSOR, 3);
    edzin_draw_lines(&buf);
    edzin_draw_statusbar(&E, &buf);
    edzin_draw_msgbar(&E, &buf);

    int line, col = 0;
    char* init_buf = NULL;

    if (block_cursor) {
        line = E.screen_props.lines + 2;
        col = (E.cursor.rx - E.scroll.x_offset) + 1;
    } else {
        line = (E.cursor.y - E.scroll.y_offset) + 1;
        col = (E.cursor.rx - E.scroll.x_offset) + 1;
    }

    init_buf = edzin_render_cursor_on_pos(line, col);
    edzin_buf_append(&buf, init_buf, strlen(init_buf));
    edzin_buf_append(&buf, ESCSEQ_SHOW_CURSOR, 6);
    write(STDOUT_FILENO, buf.buf, buf.len);
    free(init_buf);
    edzin_buf_free(&buf);
}

void
edzin_save() {
    if (E.files == NULL) {
        E.files = malloc(sizeof(edzin_file_t));
        E.files[0].filename = edzin_prompt("save as: %s", NULL);

        if (E.files[0].filename == NULL) {
            edzin_set_status_msg("save aborted");

            return;
        }

        edzin_select_syntax_highlight();
    }

    int len;
    char* buf = edzin_lines_to_str(&len);
    int fd = open(E.files[0].filename, O_RDWR | O_CREAT, 0644);

    if (fd != FAILURE) {
        if (ftruncate(fd, len) != FAILURE && write(fd, buf, len) != FAILURE) {
            close(fd);
            free(buf);
            edzin_set_status_msg("%d bytes written to disk", len);
            E.files[0].state = UNMODIFIED;
            return;
        }

        close(fd);
    }

    free(buf);
    edzin_set_status_msg("can't save .. i/o operation error: %s", strerror(errno));
}

void
edzin_scroll() {
    E.cursor.rx = 0;

    if (E.cursor.y < E.nlines) {
        E.cursor.rx = edzin_transform_x_to_rx(&E.line[E.cursor.y], E.cursor.x);
    }

    if (E.cursor.y < E.scroll.y_offset) {
        E.scroll.y_offset = E.cursor.y;
    } else if (E.cursor.y >= E.scroll.y_offset + E.screen_props.lines) {
        E.scroll.y_offset = E.cursor.y - E.screen_props.lines + 1;
    } else if (E.cursor.rx < E.scroll.x_offset) {
        E.scroll.x_offset = E.cursor.rx;
    } else if (E.cursor.rx >= E.scroll.x_offset + E.screen_props.cols) {
        E.scroll.x_offset = E.cursor.rx - E.screen_props.cols + 1;
    }
}

void
edzin_select_syntax_highlight() {
    E.syntax = NULL;

    if (E.files == NULL) {
        return;
    }

    char* ext = strrchr(E.files[0].filename, '.');

    for (unsigned int i = 0; i < HLDB_ENTRIES; i++) {
        edzin_syntax_t* s = &HLDB[i];

        for (unsigned int j = 0; s->filematch[j]; j++) {
            int is_ext = (s->filematch[j][0] == '.');

            if ((is_ext && ext && !strcmp(ext, s->filematch[j]))
                || (!is_ext && strstr(E.files[0].filename, s->filematch[j]))) {
                E.syntax = s;

                for (int line = 0; line < E.nlines; line++) {
                    edzin_update_syntax(&E.line[line]);
                }

                return;
            }
        }
    }
}

void
edzin_set_status_msg(const char* fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(E.status.msg, sizeof(E.status.msg), fmt, ap);
    va_end(ap);
    E.status.msg_time = time(NULL);
}

void
edzin_update_line(edzin_line_t* line) {
    int ntabs = 0;

    for (int i = 0; i < line->size; i++) {
        if (line->chars[i] == ASCII_TAB) {
            ntabs++;
        }
    }

    free(line->render);
    line->render = malloc(line->size + (ntabs * (TAB_STOP_SIZE - 1)) + 1);

    int idx = 0;

    for (int i = 0; i < line->size; i++) {
        if (line->chars[i] == ASCII_TAB) {
            line->render[idx++] = ASCII_SPACE;

            while (idx % TAB_STOP_SIZE != 0) {
                line->render[idx++] = ASCII_SPACE;
            }
        } else {
            line->render[idx++] = line->chars[i];
        }
    }

    line->render[idx] = ASCII_NULL;
    line->rsize = idx;
    edzin_update_syntax(line);
}

void
edzin_update_syntax(edzin_line_t* line) {
    line->highlight = realloc(line->highlight, line->rsize);
    memset(line->highlight, HL_NORMAL, line->rsize);

    if (E.syntax == NULL) {
        return;
    }

    char** keywords = E.syntax->keywords;
    char* slc = E.syntax->grammar_symbols.single_line_comment;
    char* mcb = E.syntax->grammar_symbols.multiline_comment_begin;
    char* mce = E.syntax->grammar_symbols.multiline_comment_end;
    int slclen = slc ? strlen(slc) : 0;
    int mcblen = mcb ? strlen(mcb) : 0;
    int mcelen = mce ? strlen(mce) : 0;
    int prev_sep = true;  // prev is separator
    int in_string = false;
    int in_comment = (line->idx > 0 && E.line[line->idx - 1].in_comment);

    for (int i = 0; i < line->rsize; i++) {
        char c = line->render[i];
        unsigned char prev_highlight = (i > 0) ? line->highlight[i - 1] : HL_NORMAL;

        // single line comment
        if (slclen && !in_string && !in_comment) {
            if (!strncmp(&line->render[i], slc, slclen)) {
                memset(&line->highlight[i], HL_COMMENT, line->rsize - i);
                break;
            }
        }

        // multiline comment
        if (mcblen && mcelen && !in_string) {
            if (!strncmp(&line->render[i], mcb, mcblen)) {
                memset(&line->highlight[i], HL_MLCOMMENT, mcblen);
                i++;
                in_comment = true;
                continue;
            } else if (in_comment) {
                line->highlight[i] = HL_COMMENT;

                if (!strncmp(&line->render[i], mce, mcelen)) {
                    memset(&line->highlight[i], HL_MLCOMMENT, mcelen);
                    i++;
                    in_comment = false;
                    prev_sep = true;
                    continue;
                } else {
                    continue;
                }
            }
        }

        if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                line->highlight[i] = HL_STRING;

                // escaped strings with (") char inside
                if (c == '\\' && i + 1 < line->rsize) {
                    line->highlight[i + 1] = HL_STRING;
                    i++;
                    continue;
                }

                if (c == in_string) {
                    in_string = false;
                    prev_sep = true;
                    continue;
                }
            } else {
                if (c == '"') {
                    in_string = c;
                    line->highlight[i] = HL_STRING;
                    continue;
                }
            }
        }

        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && !in_string && (prev_sep || prev_highlight == HL_NUMBER))
                || (c == '.' && prev_highlight == HL_NUMBER)) {
                line->highlight[i] = HL_NUMBER;
                prev_sep = false;
                continue;
            }
        }

        if (prev_sep) {
            int j;

            for (j = 0; keywords[j]; j++) {
                int kwlen = strlen(keywords[j]);
                int is_ktype = keywords[j][kwlen - 1] == '|';

                if (is_ktype) {
                    kwlen--;
                }

                if (!strncmp(&line->render[i], keywords[j], kwlen) && is_separator(line->render[i + kwlen])) {
                    int t = is_ktype ? HL_KEYTYPE : HL_KEYWORD;

                    memset(&line->highlight[i], t, kwlen);
                }
            }

            if (keywords[j] != NULL) {
                prev_sep = false;
                continue;
            }
        }

        prev_sep = is_separator(c);
    }

    int changed = line->in_comment != in_comment;
    line->in_comment = in_comment;

    if (changed && line->idx + 1 < E.nlines) {
        edzin_update_syntax(&E.line[line->idx + 1]);
    }
}
