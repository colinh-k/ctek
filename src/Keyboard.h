#ifndef KEYBOARD_H_
#define KEYBOARD_H_

// Functions and constants to interface with the keyboard.

typedef enum {
  // special key-binding codes.
  KEY_BACKSPACE = 500,
  // KEY_TAB,
  KEY_RETURN,
  KEY_ESC,
  // arrow keys for cursor movement.
  KEY_ARROW_RIGHT = 1000,  // ESC[C
  KEY_ARROW_LEFT,  // ESC[D
  KEY_ARROW_DOWN,  // ESC[B
  KEY_ARROW_UP,  // ESC[A
  // page up and down keys to move cursor to top and btm of screen.
  KEY_PAGE_UP,  // ESC[5~
  KEY_PAGE_DOWN,  // ESC[6~
  // home and end to move cursor to start, end of line, respectively.
  KEY_HOME,  // ESC[1~, ESC[7~, ESC[H, ESC[OH
  KEY_END,  // ESC[4~, ESC[8~, ESC[F, ESC[OF
  // delete key
  KEY_DELETE  // ESC[3~
} Key_t;

// Reads and returns 1 keypress from stdin. Calls quit if
//  reading fails or times out. Converts escape sequences
//  to single character constants defined in the above enum.
int Keyboard_ReadKey(void);

#endif  // KEYBOARD_H_
