#include "ESCCommands.h"
#include <stdio.h>

void Get_ESCCmd_Move(char *buf, int size, int col, int row) {
  snprintf((char *) buf, size, ESC_SEQ "%d;%dH", row, col);
}
