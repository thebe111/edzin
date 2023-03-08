#include "edzin/lexer.h"
#include "edzin/ascii.h"
#include <ctype.h>
#include <string.h>

int
is_separator(int c) {
    return isspace(c) || c == ASCII_NULL || strchr(",.()+-/*=~%<>[];", c) != NULL;
}
