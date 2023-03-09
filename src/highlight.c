#include "include/highlight.h"
#include <stdio.h>

static char* c_hl_extensions[] = {".c", ".h", ".cpp", NULL};
static char* c_hl_keywords[] = {"break",   "case",   "class",  "continue", "else",    "enum",      "for",   "if",
                                "return",  "static", "struct", "switch",   "typedef", "union",     "while", "char|",
                                "double|", "float|", "int|",   "long|",    "signed|", "unsigned|", "void|", NULL};

edzin_syntax_t HLDB[] = {
    {"c",
     c_hl_extensions,
     c_hl_keywords,
     {.single_line_comment = "//", .multiline_comment_begin = "/*", .multiline_comment_end = "*/"},
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};

size_t HLDB_ENTRIES = sizeof(HLDB) / sizeof(edzin_syntax_t);
