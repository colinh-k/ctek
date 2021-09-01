#include <unistd.h>
#include <ctype.h>  // for isdigit

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

    // read in '[' and next character to the buffer.
    if (WrappedRead(STDIN_FILENO, &cmd[0], 1) != 1 ||
        WrappedRead(STDIN_FILENO, &cmd[1], 1) != 1) {
      // read timed out, so user probably pressed the esc key.
      return KEY_ESC;
    }

    if (cmd[0] == (unsigned char) '[') {
      // cmd has the form: ESC[<...>
      // if <...> has a 5~ or 5~ as next 1 characters,
      //  it's a page up or down, respectively.
      if (isdigit(cmd[1])) {
        if (WrappedRead(STDIN_FILENO, &cmd[2], 1) != 1) {
          // timeout, so probably just pressed ESC key
          return KEY_ESC;
        }
        // form: ESC[<digit><...>
        if (cmd[2] == '~') {
        // form: ESC[<digit>~
          switch (cmd[1]) {
            case '3':
              return KEY_DELETE;
            case '5':  // ESC[5~
              return KEY_PAGE_UP;
            case '6':  // ESC[6~
              return KEY_PAGE_DOWN;
            case '1':
            case '7':
              // both refer to home key press (OS dependent)
              return KEY_HOME;
            case '4':
            case '8':
              // both refer to end key press (OS dependent)
              return KEY_END;
          }
        }
      } else {
        // map the arrow keys to asdw that can move the cursor.
        // form: ESC[<...>
        switch (cmd[1]) {
          case 'A':
            return KEY_ARROW_UP;
          case 'B':
            return KEY_ARROW_DOWN;
          case 'C':
            return KEY_ARROW_RIGHT;
          case 'D':
            return KEY_ARROW_LEFT;
          // other ways for OS to indicate HOME and END:
          case 'H':
            return KEY_HOME;
          case 'F':
            return KEY_END;
        }
      }
    } else if (cmd[0] == 'O') {
      // more ways for OS to indicate HOME and END:
      switch (cmd[1]) {
        case 'H':
          return KEY_HOME;
        case 'F':
          return KEY_END;
      }
    }
    return KEY_ESC;
  } else if (key == '\r') {
    return KEY_RETURN;
  } else {
    return key;
  }
}
