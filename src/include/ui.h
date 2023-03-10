#ifndef EDZIN_UI_H
#define EDZIN_UI_H

#include "main.h"
#include "smartbuf.h"

void edzin_draw_statusbar(edzin_config_t* E, edzin_smartbuf_t* buf);
void edzin_draw_msgbar(edzin_config_t* E, edzin_smartbuf_t* buf);
char* edzin_file_state_to_str(edzin_buf_state_t s);
const char* edzin_mode_to_str(edzin_mode_t mode);

#endif  // EDZIN_UI_H
