#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "include/buffer.h"
#include "include/ascii.h"
#include "include/keyboard.h"
#include "include/lexer.h"
#include "include/main.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char*
edzin_lines_to_str(int* buflen) {
    int totallen = 0;

    for (int i = 0; i < E.win->buf->nlines; i++) {
        totallen += E.win->buf->line[i].size + 1;
    }

    *buflen = totallen;

    char* buf = malloc(totallen);
    char* p = buf;

    for (int i = 0; i < E.win->buf->nlines; i++) {
        memcpy(p, E.win->buf->line[i].chars, E.win->buf->line[i].size);
        p += E.win->buf->line[i].size;
        *p = '\n';
        p++;
    }

    return buf;
}

edzin_buffer_t*
new_buffer(char* fname) {
    edzin_buffer_t* buf = malloc(sizeof(edzin_buffer_t));

    *buf = (edzin_buffer_t) {
        .idx = E.win->nbuf,
        .fname = fname ? strdup(fname) : NULL,
        .init = false,
        .line = NULL,
        .nlines = 0,
        .state = UNMODIFIED,
        .syntax = NULL,
    };

    return buf;
}

edzin_line_t
new_line(int idx, size_t size) {
    return (edzin_line_t) {
        .idx = idx,
        .size = size,
        .rsize = 0,
        .chars = malloc(size + 1),
        .render = NULL,
        .highlight = NULL,
        .in_comment = false,
    };
}

int
edzin_transform_rx_to_x(edzin_line_t* line, int chars_rx) {
    int x;
    int cur_rx = 0;

    for (x = 0; x < line->size; x++) {
        if (line->chars[x] == ASCII_HEX_TAB) {
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
        if (line->chars[i] == ASCII_HEX_TAB) {
            render_x += (TAB_STOP_SIZE - 1) - (render_x % TAB_STOP_SIZE);
        }

        render_x++;
    }

    return render_x;
}

void
edzin_backspace_char() {
    edzin_line_t* line = &E.win->buf->line[E.win->cursor.y];

    if (E.win->cursor.x > 0) {
        edzin_line_delete_char(line, E.win->cursor.x - 1);
        E.win->cursor.x--;
    }

#ifdef UO_ENABLE_DELETE_LINE_JOIN
    if (E.win->cursor.y > 0) {
        edzin_line_t* prev_line = &E.win->buf->line[E.win->cursor.y - 1];

        E.win->cursor.x = prev_line->size + line->size;
        edzin_line_append_str(prev_line, line->chars, line->size);
        edzin_delete_line(E.win->cursor.y);
        E.win->cursor.y--;
    }
#endif
}

void
edzin_clean_up_buffer(edzin_buffer_t* buf) {
    if (buf->line != NULL) {
        for (int i = 0; i < buf->nlines; i++) {
            edzin_clean_up_line(&buf->line[i]);
        }
    }

    free(buf->line);
    free(buf->fname);
    free(buf);
}

void
edzin_delete_char() {
    if (E.win->cursor.y == E.win->buf->nlines) {
        return;
    }

    if (E.win->cursor.x == 0 && E.win->cursor.y == 0) {
        return;
    }

    edzin_line_t* line = &E.win->buf->line[E.win->cursor.y];

    if (E.win->cursor.x > 0 && E.win->cursor.x < *(&E.win->buf->line[E.win->cursor.y].rsize)) {
        edzin_mv_cursor(ARROW_RIGHT);
        edzin_line_delete_char(line, E.win->cursor.x - 1);
        E.win->cursor.x--;
    }

    // FIXME
    if (E.win->cursor.y + 1 < E.win->buf->nlines) {
        edzin_line_t* next_line = &E.win->buf->line[E.win->cursor.y + 1];

        E.win->cursor.x = line->size + next_line->size;
        edzin_line_append_str(line, next_line->chars, next_line->size);
        edzin_delete_line(E.win->cursor.y + 1);
    }
}

void
edzin_delete_line(int at) {
    if (at < 0 || at >= E.win->buf->nlines) {
        return;
    }

    edzin_free_line(&E.win->buf->line[at]);
    memmove(&E.win->buf->line[at], &E.win->buf->line[at - 1], sizeof(edzin_line_t) * (E.win->buf->nlines - at - 1));

    for (int i = at; i < E.win->buf->nlines - 1; i++) {
        E.win->buf->line[i].idx--;
    }

    E.win->buf->nlines--;
    E.win->buf->state = MODIFIED;
}

void
edzin_free_line(edzin_line_t* line) {
    free(line->render);
    free(line->chars);
    free(line->highlight);
}

void
edzin_insert_char(int ch) {
    if (E.win->cursor.y == E.win->buf->nlines) {
        edzin_insert_line(E.win->buf->nlines, "", 0);
    }

    edzin_line_insert_char(&E.win->buf->line[E.win->cursor.y], E.win->cursor.x, ch);
    E.win->cursor.x++;
    E.win->buf->state = MODIFIED;
}

void
edzin_insert_line(int at, char* s, size_t len) {
    if (at < 0 || at > E.win->buf->nlines) {
        return;
    }

    if (E.win->buf->line == NULL) {
        E.win->buf->line = malloc(sizeof(edzin_line_t));
        *E.win->buf->line = new_line(at, len);
    } else {
        E.win->buf->line = realloc(E.win->buf->line, sizeof(edzin_line_t) * (E.win->buf->nlines + 1));
        memmove(&E.win->buf->line[at + 1], &E.win->buf->line[at], sizeof(edzin_line_t) * (E.win->buf->nlines - at));

        for (int i = at + 1; i <= E.win->buf->nlines; i++) {
            E.win->buf->line[i].idx++;
        }

        E.win->buf->line[at] = new_line(at, len);
    }

    memcpy(E.win->buf->line[at].chars, s, len);
    E.win->buf->line[at].chars[len] = ASCII_CHAR_NULL;
    edzin_update_line(&E.win->buf->line[at]);
    E.win->buf->nlines++;
}

void
edzin_insert_new_line() {
    if (E.win->cursor.x == 0) {
        edzin_insert_line(E.win->cursor.y, "", 0);
    } else {
        edzin_line_t* line = &E.win->buf->line[E.win->cursor.y];

        edzin_insert_line(E.win->cursor.y + 1, &line->chars[E.win->cursor.x], line->size - E.win->cursor.x);
        line = &E.win->buf->line[E.win->cursor.y];
        line->size = E.win->cursor.x;
        line->chars[line->size] = ASCII_CHAR_NULL;
        edzin_update_line(line);
    }

    E.win->cursor.y++;
    E.win->cursor.x = 0;
}

void
edzin_line_append_str(edzin_line_t* line, char* s, size_t len) {
    line->chars = realloc(line->chars, line->size + len + 1);
    memcpy(&line->chars[line->size], s, len);
    line->size += len;
    line->chars[line->size] = ASCII_CHAR_NULL;
    edzin_update_line(line);
    E.win->buf->state = MODIFIED;
}

void
edzin_line_delete_char(edzin_line_t* line, int at) {
    if (at < 0 && at >= line->size) {
        return;
    }

    memmove(&line->chars[at], &line->chars[at + 1], line->size - at);
    line->size--;
    edzin_update_line(line);
    E.win->buf->state = MODIFIED;
}

void
edzin_line_insert_char(edzin_line_t* line, int at, int ch) {
    if (at < 0 || at > line->size) {
        at = line->size;
    }

    line->chars = realloc(line->chars, line->size + 2);
    memmove(&line->chars[at + 1], &line->chars[at], line->size - at + 1);
    line->size++;
    line->chars[at] = ch;
    edzin_update_line(line);
}

void
edzin_update_line(edzin_line_t* line) {
    int ntabs = 0;

    for (int i = 0; i < line->size; i++) {
        if (line->chars[i] == ASCII_HEX_TAB) {
            ntabs++;
        }
    }

    line->render = malloc(line->size + (ntabs * (TAB_STOP_SIZE - 1)) + 1);

    int idx = 0;

    for (int i = 0; i < line->size; i++) {
        if (line->chars[i] == ASCII_HEX_TAB) {
            line->render[idx++] = ASCII_HEX_SPACE;

            while (idx % TAB_STOP_SIZE != 0) {
                line->render[idx++] = ASCII_HEX_SPACE;
            }
        } else {
            line->render[idx++] = line->chars[i];
        }
    }

    line->render[idx] = ASCII_HEX_NULL;
    line->rsize = idx;
    edzin_update_syntax(line);
}

void
edzin_update_syntax(edzin_line_t* line) {
    line->highlight = realloc(line->highlight, line->rsize);
    memset(line->highlight, HL_NORMAL, line->rsize);

    if (E.win->buf->syntax == NULL) {
        return;
    }

    char** keywords = E.win->buf->syntax->keywords;

    char* slc = E.win->buf->syntax->grammar_symbols.single_line_comment;
    char* mcb = E.win->buf->syntax->grammar_symbols.multiline_comment_begin;
    char* mce = E.win->buf->syntax->grammar_symbols.multiline_comment_end;

    int slclen = slc ? strlen(slc) : 0;
    int mcblen = mcb ? strlen(mcb) : 0;
    int mcelen = mce ? strlen(mce) : 0;

    int prev_sep = true;  // prev is separator
    int in_string = false;
    int in_comment = (line->idx > 0 && E.win->buf->line[line->idx - 1].in_comment);

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

        if (E.win->buf->syntax->flags & HL_HIGHLIGHT_STRINGS) {
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

        if (E.win->buf->syntax->flags & HL_HIGHLIGHT_NUMBERS) {
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

    if (changed && line->idx + 1 < E.win->buf->nlines) {
        edzin_update_syntax(&E.win->buf->line[line->idx + 1]);
    }
}

void
edzin_select_syntax_highlight() {
    E.win->buf->syntax = NULL;

    if (E.win->buf->fname == NULL) {
        return;
    }

    char* ext = strrchr(E.win->buf->fname, '.');

    for (unsigned int i = 0; i < HLDB_ENTRIES; i++) {
        edzin_syntax_t* s = &HLDB[i];

        for (unsigned int j = 0; s->filematch[j]; j++) {
            int is_ext = (s->filematch[j][0] == '.');

            if ((is_ext && ext && !strcmp(ext, s->filematch[j]))
                || (!is_ext && strstr(E.win->buf->fname, s->filematch[j]))) {
                E.win->buf->syntax = s;

                for (int line = 0; line < E.win->buf->nlines; line++) {
                    edzin_update_syntax(&E.win->buf->line[line]);
                }

                return;
            }
        }
    }
}

void
edzin_clean_up_line(edzin_line_t* line) {
    free(line->chars);
    free(line->render);
    free(line->highlight);
}

void
edzin_clean_up_syntax(edzin_syntax_t* syntax) {
    free(syntax->keywords);
    free(syntax->filetype);
    free(syntax->filematch);
    free(syntax);
}
