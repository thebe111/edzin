#ifndef EDZIN_ASCII_H
#define EDZIN_ASCII_H

#define ASCII_SPACE 0x20
#define ASCII_TAB 0x09

typedef struct {
    char cname[30];  // char name
    char gchar;  // graphical char representation
    int hexcode;
} ascii_char_t;

#endif  // EDZIN_ASCII_H
