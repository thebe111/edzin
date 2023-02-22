#include "edzin/main.h"
#include "edzin/user_options.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

edzin_config edzin_defconfig;

int
main(__attribute((unused)) int argc, __attribute__((unused)) char** argv) {
    enable_raw_mode();
    edzin_init();

    while (1) {
        edzin_refresh_screen();
        edzin_process_keypress();
    }

    return EXIT_SUCCESS;
}

void
disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &edzin_defconfig.TERM_MODE) == ERROR) {
        edzin_die("tcsetattr");
    }
}

void
enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &edzin_defconfig.TERM_MODE) == ERROR) {
        edzin_die("tcgetattr");
    }

    atexit(disable_raw_mode);
    struct termios term_raw_mode = edzin_defconfig.TERM_MODE;
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

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_raw_mode) == ERROR) {
        edzin_die("tcsetattr");
    }
}

int
get_cursor_pos(int* rows, int* cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return ERROR;
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
        return ERROR;
    }

    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return ERROR;
    }

    return EXIT_SUCCESS;
}

int
get_winsize(int* rows, int* cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == ERROR || ws.ws_col == 0) {
        // hard way to get all terminals screen size, moving the cursor to the bottom-right corner
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return get_cursor_pos(rows, cols);
        }

        edzin_read_key();

        return ERROR;
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return EXIT_SUCCESS;
}

void
buf_append(edzin_append_buf* buf, const char* s, int len) {
    char* new = realloc(buf->buf, buf->len + len);

    if (new == NULL) {
        return;
    }

    memcpy(&new[buf->len], s, len);
    buf->buf = new;
    buf->len += len;
}

void
buf_free(edzin_append_buf* buf) {
    free(buf->buf);
}

int
edzin_read_key() {
    char c;
    int nread;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == ERROR && errno != EAGAIN) {
            edzin_die("read");
        }
    }

    if (c == ESCAPE) {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1 || read(STDIN_FILENO, &seq[1], 1) != 1) {
            return ESCAPE;
        }

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                    return uo_arrow_keys(seq[1]);
            }
        }

        return ESCAPE;
    }

    return c;
}

void
edzin_die(const char* msg) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(msg);
    exit(EXIT_FAILURE);
}

void
edzin_draw_rows(edzin_append_buf* buf) {
    for (int i = 0; i < edzin_defconfig.screen_props.rows; i++) {
        if (i == edzin_defconfig.screen_props.rows / 3) {
            char greatings_msg[80];
            int greatings_len = snprintf(greatings_msg, sizeof(greatings_msg), "Edzin :: v%s\r\n", EDZIN_VERSION);

            if (greatings_len > edzin_defconfig.screen_props.cols) {
                greatings_len = edzin_defconfig.screen_props.cols;
            }

            int padding = (edzin_defconfig.screen_props.cols - greatings_len) / 2;

            if (padding) {
                buf_append(buf, "~", 1);
                padding--;
            }

            while (padding--) {
                buf_append(buf, " ", 1);
            }

            buf_append(buf, greatings_msg, greatings_len);
        } else {
            buf_append(buf, "~", 1);
        }

        buf_append(buf, "\x1b[K", 3);

        if (i < edzin_defconfig.screen_props.rows - 1) {
            buf_append(buf, "\r\n", 2);
        }
    }
}

void
edzin_init() {
    edzin_defconfig.cursor.x = 0;
    edzin_defconfig.cursor.y = 0;

    if (get_winsize(&edzin_defconfig.screen_props.rows, &edzin_defconfig.screen_props.cols) == ERROR) {
        edzin_die("get_winsize");
    }
}

void
edzin_move_cursor(int key) {
    switch (key) {
        case 'h':
        case ARROW_LEFT:
            if (edzin_defconfig.cursor.x > 0) {
                edzin_defconfig.cursor.x--;
            }

            break;
        case 'j':
        case ARROW_DOWN:
            if (edzin_defconfig.cursor.y < edzin_defconfig.screen_props.rows - 1) {
                edzin_defconfig.cursor.y++;
            }

            break;
        case 'k':
        case ARROW_UP:
            if (edzin_defconfig.cursor.y > 0) {
                edzin_defconfig.cursor.y--;
            }

            break;
        case 'l':
        case ARROW_RIGHT:
            if (edzin_defconfig.cursor.x < edzin_defconfig.screen_props.cols - 1) {
                edzin_defconfig.cursor.x++;
            }

            break;
    }
}

void
edzin_process_keypress() {
    int c = edzin_read_key();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(EXIT_SUCCESS);
            break;
        case 'h':
        case ARROW_LEFT:
        case 'j':
        case ARROW_DOWN:
        case 'k':
        case ARROW_UP:
        case 'l':
        case ARROW_RIGHT:
            edzin_move_cursor(c);
            break;
    }
}

void
edzin_refresh_screen() {
    edzin_append_buf buf = APPEND_BUF_INIT;

    buf_append(&buf, "\x1b[?25l", 6);  // hide cursor before rendering
    buf_append(&buf, "\x1b[H", 3);  // vt100 escape sequence to go to line 1, col 1
    edzin_draw_rows(&buf);

    char init_buf[32];

    snprintf(init_buf, sizeof(init_buf), "\x1b[%d;%dH", edzin_defconfig.cursor.y + 1, edzin_defconfig.cursor.x + 1);
    buf_append(&buf, init_buf, strlen(init_buf));
    buf_append(&buf, "\x1b[?25h", 6);  // show cursor after rendering
    write(STDOUT_FILENO, buf.buf, buf.len);
    buf_free(&buf);
}
