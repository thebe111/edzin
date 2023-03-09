#ifndef EDZIN_HANDLERS_H
#define EDZIN_HANDLERS_H

#include "main.h"

int handle_arrow_keys(char ch);
void handle_mv_cursor_down(edzin_config_t* E);
void handle_mv_cursor_left(edzin_config_t* E);
void handle_mv_cursor_right(edzin_config_t* E, edzin_line_t* line);
void handle_mv_cursor_up(edzin_config_t* E);
void handle_page_down(edzin_config_t* E, int ch);

#endif  // EDZIN_HADLERS_H
