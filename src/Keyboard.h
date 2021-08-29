#ifndef KEYBOARD_H_
#define KEYBOARD_H_

// Functions and constants to interface with the keyboard.

typedef enum {
  ARROW_RIGHT = 1000,
  ARROW_LEFT,
  ARROW_DOWN,
  ARROW_UP
} key_t;

// Reads and returns 1 keypress from stdin. Calls quit if
//  reading fails or times out. Converts escape sequences
//  to single character constants defined in the above enum.
int Keyboard_ReadKey(void);

#endif  // KEYBOARD_H_
