#include <unistd.h>

#include "Keyboard.h"
#include "Quit.h"
#include "IOUtils.h"
#include "ESCCommands.h"

int Keyboard_ReadKey(void) {
  int bytes_read;
  unsigned char key;
  while ((bytes_read = WrappedRead(STDIN_FILENO, &key, 1)) != 1) {
    if (bytes_read == -1) quit("Keyboard_ReadKey");
  }

  // read an escape sequence as 1 character
  if (key == (unsigned char) ESC) {
    unsigned char cmd[3];

    if (WrappedRead(STDIN_FILENO, &cmd[0], 1) != 1 ||
        WrappedRead(STDIN_FILENO, &cmd[1], 1) != 1) {
      // read timed out, so user probably pressed the esc key.
      return ESC;
    }

    if (cmd[0] == (unsigned char) '[') {
      // map the arrow keys to asdw that can move the cursor.
      switch (cmd[1]) {
        case 'A':
          return ARROW_UP;
        case 'B':
          return ARROW_DOWN;
        case 'C':
          return ARROW_RIGHT;
        case 'D':
          return ARROW_LEFT;
      }
    }
    return ESC;
  } else {
    return key;
  }
}
