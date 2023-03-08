#ifndef EDZIN_UI_H
#define EDZIN_UI_H

#include "main.h"

void edzin_draw_statusbar(edzin_config_t* E, edzin_append_buf_t* buf);
void edzin_draw_msgbar(edzin_config_t* E, edzin_append_buf_t* buf);
char* edzin_file_state_to_str(enum edzin_file_state_e s);

#endif  // EDZIN_UI_H
