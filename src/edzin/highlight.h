#ifndef EDZIN_HIGHLIGHT_H
#define EDZIN_HIGHLIGHT_H

#include <stddef.h>

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

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
