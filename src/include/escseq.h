#ifndef EDZIN_ESCSEQ_H
#define EDZIN_ESCSEQ_H

#define ESCSEQ_CLEAR_LINE "\x1b[K"
#define ESCSEQ_CLEAR_SCREEN "\x1b[2J"
#define ESCSEQ_COLOR "\x1b[%dm"
#define ESCSEQ_COLOR_DEF "\x1b[39m"
#define ESCSEQ_HIDE_CURSOR "\x1b[?25l"
#define ESCSEQ_MOVE_CURSOR "\x1b[%d;%dH"
#define ESCSEQ_RESET_CURSOR "\x1b[H"
#define ESCSEQ_REVERT_COLORS "\x1b[7m"
#define ESCSEQ_SHOW_CURSOR "\x1b[?25h"

#endif  // EDZIN_ESCSEQ_H
