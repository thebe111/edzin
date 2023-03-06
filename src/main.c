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
#include "edzin/handlers.h"
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

    edzin_set_status_msg("HELP: Ctrl+S = save | Ctrl+Q = quit");

    while (true) {
        edzin_refresh_screen();
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
edzin_prompt(char* prompt) {
    size_t bufsize = 128;
    char* buf = malloc(bufsize);
    size_t buflen = 0;

    buf[0] = '\0';

    while (true) {
        edzin_set_status_msg(prompt, buf);
        edzin_refresh_screen();

        int c = edzin_read_key();

        if (c == DELETE_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = '\0';
            }
        } else if (c == ESCAPE) {
            edzin_set_status_msg("");
            free(buf);

            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                edzin_set_status_msg("");

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
    }
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
    E.line[at].size = len;
    E.line[at].chars = malloc(len + 1);
    memcpy(E.line[at].chars, s, len);
    E.line[at].chars[len] = '\0';
    E.line[at].rsize = 0;
    E.line[at].render = NULL;
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
        line->chars[line->size] = '\0';
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
    E.nlines--;

    if (E.files != NULL) {
        E.files[0].state = MODIFIED;
    }
}

void
edzin_die(const char* msg) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
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

            edzin_buf_append(buf, &E.line[file_line].render[E.scroll.x_offset], len);
        }

        edzin_buf_append(buf, "\x1b[K", 3);
        edzin_buf_append(buf, "\r\n", 2);
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
edzin_free_line(edzin_line_t* line) {
    free(line->render);
    free(line->chars);
}

void
edzin_init() {
    E.cursor.x = 0;
    E.cursor.y = 0;
    E.cursor.rx = 0;
    E.scroll.x_offset = 0;
    E.scroll.y_offset = 0;
    E.nlines = 0;
    E.line = NULL;
    E.nfiles = 0;
    E.files = NULL;
    E.status.msg[0] = '\0';
    E.status.msg_time = 0;

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
    line->chars[line->size] = '\0';
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
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(EXIT_SUCCESS);
            break;
        case CTRL_KEY('s'):
            edzin_save();
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
        case '\x1b':
            break;
        default:
            edzin_insert_char(c);
            break;
    }
}

void
edzin_refresh_screen() {
    edzin_scroll();

    edzin_append_buf_t buf = APPEND_BUF_INIT;

    edzin_buf_append(&buf, "\x1b[?25l", 6);  // hide cursor before rendering
    edzin_buf_append(&buf, "\x1b[H", 3);  // vt100 escape sequence to go to line 1, col 1
    edzin_draw_lines(&buf);
    edzin_draw_statusbar(&E, &buf);
    edzin_draw_msgbar(&E, &buf);

    char init_buf[32];

    snprintf(init_buf,
             sizeof(init_buf),
             "\x1b[%d;%dH",
             (E.cursor.y - E.scroll.y_offset) + 1,
             (E.cursor.rx - E.scroll.x_offset) + 1);
    edzin_buf_append(&buf, init_buf, strlen(init_buf));
    edzin_buf_append(&buf, "\x1b[?25h", 6);  // show cursor after rendering
    write(STDOUT_FILENO, buf.buf, buf.len);
    edzin_buf_free(&buf);
}

void
edzin_save() {
    if (E.files == NULL) {
        E.files = malloc(sizeof(edzin_file_t));
        E.files[0].filename = edzin_prompt("save as: %s");

        if (E.files[0].filename == NULL) {
            edzin_set_status_msg("save aborted");

            return;
        }
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

    line->render[idx] = 0x00;
    line->rsize = idx;
}
