#ifndef KEYBOARD_H_
#define KEYBOARD_H_

// Functions and constants to interface with the keyboard.

typedef enum {
  // arrow keys for cursor movement.
  ARROW_RIGHT = 1000,  // ESC[C
  ARROW_LEFT,  // ESC[D
  ARROW_DOWN,  // ESC[B
  ARROW_UP,  // ESC[A
  // page up and down keys to move cursor to top and btm of screen.
  PAGE_UP,  // ESC[5~
  PAGE_DOWN,  // ESC[6~
  // home and end to move cursor to start, end of line, respectively.
  HOME,  // ESC[1~, ESC[7~, ESC[H, ESC[OH
  END,  // ESC[4~, ESC[8~, ESC[F, ESC[OF
  // delete key
  DELETE  // ESC[3~
} Key_t;

// Reads and returns 1 keypress from stdin. Calls quit if
//  reading fails or times out. Converts escape sequences
//  to single character constants defined in the above enum.
int Keyboard_ReadKey(void);

#endif  // KEYBOARD_H_
