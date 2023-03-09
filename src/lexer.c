#include "include/lexer.h"
#include "include/ascii.h"
#include <ctype.h>
#include <string.h>

int
is_separator(int ch) {
    return isspace(ch) || ch == ASCII_HEX_NULL || strchr(",.()+-/*=~%<>[];", ch) != NULL;
}
