#ifndef EDZIN_HANDLERS_H
#define EDZIN_HANDLERS_H

#include "main.h"

int handle_arrow_keys(char key);
void handle_mv_cursor_down(edzin_config_t* E);
void handle_mv_cursor_left(edzin_config_t* E);
void handle_mv_cursor_right(edzin_config_t* E, edzin_row_t* row);
void handle_mv_cursos_up(edzin_config_t* E);

#endif  // EDZIN_HADLERS_H
