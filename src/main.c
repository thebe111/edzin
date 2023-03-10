#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "include/main.h"
#include "include/ascii.h"
#include "include/escseq.h"
#include "include/handlers.h"
#include "include/keyboard.h"
#include "include/ui.h"
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
        edzin_process_keypress(E.mode);
    }

    return EXIT_SUCCESS;
}
#endif

char*
edzin_prompt(char* prompt, void (*cb)(char* query, int ch)) {
    size_t bufsize = 128;
    char* buf = malloc(bufsize);
    size_t buflen = 0;
    bool block_cursor = true;

    buf[0] = ASCII_CHAR_NULL;

    while (true) {
        edzin_set_status_msg(prompt, buf);
        E.win->cursor.x = buflen + strlen(prompt) - 2;
        edzin_refresh_screen(block_cursor);

        int ch = edzin_read_key();

        if (ch == DELETE_KEY || ch == CTRL_KEY('h') || ch == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = ASCII_CHAR_NULL;
            }
        } else if (ch == ASCII_CHAR_ESCAPE) {
            block_cursor = false;
            edzin_set_status_msg("");

            if (cb) {
                cb(buf, ch);
            }

            free(buf);

            return NULL;
        } else if (ch == ASCII_CHAR_CARRIAGE_RET) {
            block_cursor = false;

            if (buflen != 0) {
                edzin_set_status_msg("");

                if (cb) {
                    cb(buf, ch);
                }

                return buf;
            }
        } else if (!iscntrl(ch) && ch < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }

            buf[buflen++] = ch;
            buf[buflen] = ASCII_CHAR_NULL;
        }

        if (cb) {
            cb(buf, ch);
        }
    }
}

int
edzin_get_scrsize(int* lines, int* cols) {
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

    if (c == ASCII_CHAR_ESCAPE) {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1 || read(STDIN_FILENO, &seq[1], 1) != 1) {
            return ASCII_CHAR_ESCAPE;
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return ASCII_CHAR_ESCAPE;
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

        return ASCII_CHAR_ESCAPE;
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

void
edzin_clean_up() {
    edzin_clean_up_window(E.win);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.TERM_MODE) == FAILURE) {
        edzin_die("tcsetattr");
    }
}

void
edzin_cmode_process_keypress(int ch) {
    UNUSED(ch);
}

void
edzin_common_process_keypress(int ch) {
    switch (ch) {
        case ASCII_CHAR_CARRIAGE_RET:
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
        case 'g':
            handle_goto(&E);
            break;
        case 'G':
            E.win->cursor.y = E.win->buf->nlines;
            break;
        case HOME_KEY:
            E.win->cursor.x = 0;
            break;
        case END_KEY:
        case '$': {
            if (E.win->cursor.y < E.win->buf->nlines) {
                E.win->cursor.x = E.win->buf->line[E.win->cursor.y].size;
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
            handle_page_down(&E, ch);
            break;
        case 'h':
        case ARROW_LEFT:
        case 'j':
        case ARROW_DOWN:
        case 'k':
        case ARROW_UP:
        case 'l':
        case ARROW_RIGHT:
            edzin_mv_cursor(ch);
            break;
        case CTRL_KEY('l'):
        case ASCII_CHAR_ESCAPE:
            E.mode = NORMAL;
            break;
        default:
            edzin_insert_char(ch);
            break;
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
edzin_draw_lines(edzin_smartbuf_t* buf) {
    for (int i = 0; i < E.scr_props.lines; i++) {
        int file_line = i + E.win->scroll.y_offset;

        if (file_line >= E.win->buf->nlines) {
            bool display_greatings = E.win->buf->nlines == 0;

            if (display_greatings && i == E.scr_props.lines / 3) {
                char greatings_msg[80];
                int greatings_len = snprintf(greatings_msg, sizeof(greatings_msg), "Edzin :: v%s\r\n", EDZIN_VERSION);

                if (greatings_len > E.scr_props.cols) {
                    greatings_len = E.scr_props.cols;
                }

                int padding = (E.scr_props.cols - greatings_len) / 2;

                if (padding) {
                    edzin_sbuf_append(buf, "~", 1);
                    padding--;
                }

                while (padding--) {
                    edzin_sbuf_append(buf, " ", 1);
                }

                edzin_sbuf_append(buf, greatings_msg, greatings_len);
            } else {
                edzin_sbuf_append(buf, "~", 1);
            }
        } else {
            int len = E.win->buf->line[file_line].rsize - E.win->scroll.x_offset;

            if (len < 0) {
                len = 0;
            }

            if (len > E.scr_props.cols) {
                len = E.scr_props.cols;
            }

            char* c = &E.win->buf->line[file_line].render[E.win->scroll.x_offset];
            unsigned char* highlight = &E.win->buf->line[file_line].highlight[E.win->scroll.x_offset];

            for (int i = 0; i < len; i++) {
#if 0  // @@@
                if (iscntrl(c[i])) {
                    char sym = (c[i] <= 26) ? '@' + c[i] : '?';

                    edzin_sbuf_append(buf, ESCSEQ_REVERT_COLORS, 4);
                    edzin_sbuf_append(buf, &sym, 1);
                    edzin_sbuf_append(buf, "\x1b[m", 3);
                }
#endif

                if (highlight[i] == HL_NORMAL) {
                    edzin_sbuf_append(buf, ESCSEQ_COLOR_DEF, 5);
                    edzin_sbuf_append(buf, &c[i], 1);
                } else {
                    static int COLOR_BUFLEN = 16;
                    char color_buf[COLOR_BUFLEN];
                    int color = edzin_syntax_to_color(highlight[i]);
                    int colorlen = snprintf(color_buf, COLOR_BUFLEN, ESCSEQ_COLOR, color);

                    edzin_sbuf_append(buf, color_buf, colorlen);
                    edzin_sbuf_append(buf, &c[i], 1);
                }
            }

            edzin_sbuf_append(buf, ESCSEQ_COLOR_DEF, 5);
        }

        edzin_sbuf_append(buf, ESCSEQ_CLEAR_LINE, 3);
        edzin_sbuf_append(buf, BREAK_LINE, 2);
    }
}

void
edzin_enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &E.TERM_MODE) == FAILURE) {
        edzin_die("tcgetattr");
    }

    int err = atexit(edzin_clean_up);

    if (err) {
        fprintf(stderr, "ERR: cannot set exit function\n");
        exit(EXIT_FAILURE);
    }

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
        .x = E.win->cursor.x,
        .y = E.win->cursor.y,
    };

    edzin_scroll_t bkp_scroll = {
        .x_offset = E.win->scroll.x_offset,
        .y_offset = E.win->scroll.y_offset,
    };

    char* query = edzin_prompt("query: %s", edzin_find_callback);

    if (query) {
        free(query);

        return;
    }

    E.win->cursor.x = bkp_cursor.x;
    E.win->cursor.y = bkp_cursor.y;
    E.win->scroll.x_offset = bkp_scroll.x_offset;
    E.win->scroll.y_offset = bkp_scroll.y_offset;
}

void
edzin_find_callback(char* query, int ch) {
    static int prev_highlight_line;
    static char* prev_highlight = NULL;

    if (prev_highlight) {
        memcpy(E.win->buf->line[prev_highlight_line].highlight,
               prev_highlight,
               E.win->buf->line[prev_highlight_line].rsize);
        free(prev_highlight);
        prev_highlight = NULL;
    }

    if (ch == ASCII_CHAR_ESCAPE) {
        return;
    }

    for (int i = 0; i < E.win->buf->nlines; i++) {
        edzin_line_t* line = &E.win->buf->line[i];
        char* match = strstr(line->render, query);

        if (match) {
            E.win->cursor.y = i;
            E.win->cursor.x = edzin_transform_rx_to_x(line, match - line->render);
            E.win->scroll.y_offset = E.win->buf->nlines;
            prev_highlight_line = i;
            prev_highlight = malloc(line->rsize);
            memcpy(prev_highlight, line->highlight, line->rsize);
            memset(&line->highlight[match - line->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void
edzin_imode_process_keypress(int ch) {
    switch (ch) {
        case 'h':  // override
        case 'j':  // override
        case 'k':  // override
        case 'l':  // override
            edzin_insert_char(ch);
            break;
        case '$':  // override
            edzin_insert_char(ch);
            break;
        default:
            edzin_common_process_keypress(ch);
            break;
    }
}

void
edzin_init() {
    E.mode = NORMAL;
    E.win = new_window();
    E.win->buf = new_buffer(NULL);
    E.win->buf->init = true;
    E.status = (edzin_status_t) {
        .msg = {ASCII_CHAR_NULL},
        .msg_time = 0,
    };

    if (edzin_get_scrsize(&E.scr_props.lines, &E.scr_props.cols) == FAILURE) {
        edzin_die("get_scrsize");
    }

    E.scr_props.lines -= 2;
}

void
edzin_nmode_process_keypress(int ch) {
    switch (ch) {
        case ':':
            E.mode = COMMAND;
            break;
        case 'i':
            E.mode = INSERT;
            break;
        case 'v':
            E.mode = VISUAL;
            break;
        case ASCII_CHAR_CARRIAGE_RET:
            edzin_mv_cursor('j');
            break;
        default:
            edzin_common_process_keypress(ch);
            break;
    }
}

void
edzin_open(char* fname) {
    E.win->nbuf += 1;

    if (E.win->buf->init) {
        E.win->buf->fname = strdup(fname);
    } else {
        E.win->buf = new_buffer(fname);
    }

    edzin_select_syntax_highlight();

    FILE* f = fopen(fname, "r");

    if (!f) {
        edzin_die("fopen");
    }

    ssize_t linelen;
    char* line = NULL;
    size_t line_capacity = 0;

    while ((linelen = getline(&line, &line_capacity, f)) != FAILURE) {
        while (linelen > 0
               && (line[linelen - 1] == ASCII_CHAR_NEW_LINE || line[linelen - 1] == ASCII_CHAR_CARRIAGE_RET)) {
            linelen--;
        }

        edzin_insert_line(E.win->buf->nlines, line, linelen);
    }

    free(line);
    fclose(f);
}

void
edzin_process_keypress(edzin_mode_t mode) {
    int ch = edzin_read_key();

    switch (mode) {
        case COMMAND:
            edzin_cmode_process_keypress(ch);
            break;
        case VISUAL:
            edzin_vmode_process_keypress(ch);
            break;
        case INSERT:
            edzin_imode_process_keypress(ch);
            break;
        default:
            edzin_nmode_process_keypress(ch);
            break;
    }
}

void
edzin_refresh_screen(bool block_cursor) {
    edzin_scroll();

    edzin_smartbuf_t buf = SMART_BUF_INIT;

    edzin_sbuf_append(&buf, ESCSEQ_HIDE_CURSOR, 6);
    edzin_sbuf_append(&buf, ESCSEQ_RESET_CURSOR, 3);
    edzin_draw_lines(&buf);
    edzin_draw_statusbar(&E, &buf);
    edzin_draw_msgbar(&E, &buf);

    int line, col = 0;
    char* init_buf = NULL;

    if (block_cursor) {
        line = E.scr_props.lines + 2;
        col = (E.win->cursor.rx - E.win->scroll.x_offset) + 1;
    } else {
        line = (E.win->cursor.y - E.win->scroll.y_offset) + 1;
        col = (E.win->cursor.rx - E.win->scroll.x_offset) + 1;
    }

    init_buf = edzin_render_cursor_on_pos(line, col);
    edzin_sbuf_append(&buf, init_buf, strlen(init_buf));
    edzin_sbuf_append(&buf, ESCSEQ_SHOW_CURSOR, 6);
    write(STDOUT_FILENO, buf.buf, buf.len);
    free(init_buf);
    edzin_sbuf_free(&buf);
}

void
edzin_save() {
    if (E.win->buf->fname == NULL) {
        E.win->buf = malloc(sizeof(edzin_buffer_t));
        E.win->buf->fname = edzin_prompt("save as: %s", NULL);

        if (E.win->buf->fname == NULL) {
            edzin_set_status_msg("save aborted");

            return;
        }

        edzin_select_syntax_highlight();
    }

    int len;
    char* buf = edzin_lines_to_str(&len);
    int fd = open(E.win->buf->fname, O_RDWR | O_CREAT, 0644);

    if (fd != FAILURE) {
        if (ftruncate(fd, len) != FAILURE && write(fd, buf, len) != FAILURE) {
            close(fd);
            free(buf);
            edzin_set_status_msg("%d bytes written to disk", len);
            E.win->buf->state = UNMODIFIED;
            return;
        }

        close(fd);
    }

    free(buf);
    edzin_set_status_msg("can't save .. i/o operation error: %s", strerror(errno));
}

void
edzin_scroll() {
    E.win->cursor.rx = 0;

    if (E.win->cursor.y < E.win->buf->nlines) {
        E.win->cursor.rx = edzin_transform_x_to_rx(&E.win->buf->line[E.win->cursor.y], E.win->cursor.x);
    }

    if (E.win->cursor.y < E.win->scroll.y_offset) {
        E.win->scroll.y_offset = E.win->cursor.y;
    } else if (E.win->cursor.y >= E.win->scroll.y_offset + E.scr_props.lines) {
        E.win->scroll.y_offset = E.win->cursor.y - E.scr_props.lines + 1;
    } else if (E.win->cursor.rx < E.win->scroll.x_offset) {
        E.win->scroll.x_offset = E.win->cursor.rx;
    } else if (E.win->cursor.rx >= E.win->scroll.x_offset + E.scr_props.cols) {
        E.win->scroll.x_offset = E.win->cursor.rx - E.scr_props.cols + 1;
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
edzin_vmode_process_keypress(int ch) {
    switch (ch) {
        case ASCII_CHAR_CARRIAGE_RET:
            edzin_mv_cursor('j');
            break;
        default:
            edzin_common_process_keypress(ch);
            break;
    }
}
