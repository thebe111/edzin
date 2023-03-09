#ifndef EDZIN_HIGHLIGHT_H
#define EDZIN_HIGHLIGHT_H

#include <stddef.h>

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

typedef enum {
    HL_COMMENT,
    HL_KEYTYPE,
    HL_KEYWORD,
    HL_MATCH,
    HL_MLCOMMENT,
    HL_NORMAL,
    HL_NUMBER,
    HL_STRING,
} edzin_highlight_types_t;

typedef enum {
    HL_RED = 31,
    HL_GREEN = 32,
    HL_YELLOW = 33,
    HL_BLUE = 34,
    HL_MAGENTA = 35,
    HL_CYAN = 36,
    HL_WHITE = 37,
} edzin_highlight_colors_t;

struct highlight_grammar {
    char* single_line_comment;
    char* multiline_comment_begin;
    char* multiline_comment_end;
};

typedef struct {
    char* filetype;
    char** filematch;
    char** keywords;
    struct highlight_grammar grammar_symbols;
    int flags;
} edzin_syntax_t;

extern size_t HLDB_ENTRIES;
extern edzin_syntax_t HLDB[];

#endif  // EDZIN_HIGHLIGHT_H
