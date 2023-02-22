#include "edzin/user_options.h"
#include "edzin/main.h"

int
uo_arrow_keys(char key) {
    if (UO_ARROW_KEYS) {
        switch (key) {
            case 'A':
                return ARROW_UP;
            case 'B':
                return ARROW_DOWN;
            case 'C':
                return ARROW_RIGHT;
            case 'D':
                return ARROW_LEFT;
        }
    } else {
        return ESCAPE;
    }
}
