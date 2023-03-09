#ifndef EZIN_KEYBOARD_H
#define EZIN_KEYBOARD_H

#define CTRL_KEY(key) ((key) &0x1f)
#define TAB_STOP_SIZE 4

typedef enum {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DELETE_KEY,
} edzin_key_t;

#endif  // EZIN_KEYBOARD_H
