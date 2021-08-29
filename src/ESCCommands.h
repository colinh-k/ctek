#ifndef ESC_COMMANDS_H_
#define ESC_COMMANDS_H_

// a collection of escape sequences for sending
//  commands to the terminal through stdin.

// the escape character, which preceeds all escape sequences for
//  terminal output.
#define ESC '\x1b'
// start of an escape sequence.
#define ESC_SEQ "\x1b["

// scope args:
#define SCREEN "J"  // target whole screen.
#define LINE "K"    // target the current line.
// to args:
#define ALL "2"    // clear the whole screen.
#define START "1"  // clear up to the cursor from start.
#define END "0"    // clear from the cursor to the end.
// clear the given scope to the given position.
#define ESC_CMD_CLEAR(scope, to) (ESC_SEQ to scope)

// origin is at the top left of the terminal screen.
#define ORIGIN "1;1"
#define ESC_CMD_MOVE(pos) (ESC_SEQ pos "H")

// hide or show the cursor.
#define HIDE "l"
#define SHOW "h"
// pass HIDE or SHOW to hide or show cursor.
#define ESC_CMD_CUR_MODE(m) (ESC_SEQ "?25" m)

// Puts an esc command sequence for moving the cursor to the
//  given row and column in given buffer.
void Get_ESCCmd_Move(unsigned char *buf, int size, int col, int row);

#endif  // ESC_COMMANDS_H_
