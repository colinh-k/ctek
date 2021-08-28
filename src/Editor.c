#include <string.h>
#include "Quit.h"
#include "Editor.h"
#include "WriteBuffer.h"

// macros

// in raw mode, terminal does not automatically convert \n to \r\n,
//  so this is used to represent newline characters.
#define NL "\r\n"
// the character that appears on the left of an empty line.
#define EMPTY_LN ">"

// converts a character byte to the ctrl version of that key.
// sets highest 3 bits to 0 (i.e., 0x1F == 0b00011111).
#define CHAR_TO_CTRL(key) ((key) & 0x1F)

// the escape character to preceed all escape sequences for
//  terminal output.
#define ESC "\x1b"
// start an escape sequence
#define ESC_SEQ "\x1b["
// arg to clear the whole screen.
#define ALL "2"
// arg to clear up to the cursor from start.
#define START "1"
// arg to clear from the cursor to the end.
#define END "0"
// clear the screen by the given argument.
#define ESC_CLEAR(x) (ESC "[" x "J")

// origin is at the top left of the terminal screen.
#define ORIGIN "1;1"
#define ESC_MOVE(pos) (ESC "[" pos "H")

// hide or show the cursor.
#define HIDE "l"
#define SHOW "h"
// pass HIDE or SHOW to make a hide or show cursor esc sequence.
#define CUR_MODE(m) (ESC "[?25" m)

// global editor state struct.
typedef struct editor_state {
  // the original terminal attributes.
  struct termios og_term_attr;
  // editor window size.
  int num_rows;
  int num_cols;
} EditorState;

static EditorState e_state;

// global write buffer.
// Buffer write_buf;

// static helper functions.

// moves cursor to the origin
static void Editor_ToOrigin(Buffer *wbuf);
// draw rows starting with ">"
static void Editor_RenderRows(Buffer *wbuf);
// send the given ESC sequence to stdout.
static void Editor_AppendEscCmd(Buffer *wbuf, const char *cmd);

void Editor_Open(void) {
  // enable raw mode.
  Term_SetRawMode(&e_state.og_term_attr);
  // get the size of the terminal window.
  int res = Term_Size(&e_state.num_rows, &e_state.num_cols);
  if (res == -1) quit("Term_Size");
}

void Editor_Close(void) {
  Term_UnSetRawMode(&e_state.og_term_attr);
}

void Editor_InterpretKeypress(void) {
  char c = ReadKey();

  switch (c) {
    case CHAR_TO_CTRL('q'):
      Editor_Refresh();
      Editor_Close();
      exit(EXIT_SUCCESS);
      break;
  }
}

void Editor_Refresh(void) {
  Buffer write_buf = EMPTY_BUF;

  // hide the cursor while rendering.
  Editor_AppendEscCmd(&write_buf, CUR_MODE(HIDE));

  // clear the screen.
  Editor_AppendEscCmd(&write_buf, ESC_CLEAR(ALL));
  // WrappedWrite(STDOUT_FILENO,
  //                     (unsigned char *) ESC_CLEAR(ALL),
  //                     strlen(ESC_CLEAR(ALL)));
  Editor_ToOrigin(&write_buf);
  Editor_RenderRows(&write_buf);
  Editor_ToOrigin(&write_buf);

  // show the cursor.
  Editor_AppendEscCmd(&write_buf, CUR_MODE(SHOW));

  // flush the buffer by writing all commands to stdout.
  WrappedWrite(STDOUT_FILENO, write_buf.buffer, write_buf.size);
  // free the buffer.
  WriteBuffer_Free(&write_buf);
}

static void Editor_RenderRows(Buffer *wbuf) {
  for (int y = 0; y < e_state.num_rows; y++) {
    Editor_AppendEscCmd(wbuf, EMPTY_LN);
    // WriteBuffer_Append(wbuf, EMPTY_LN, strlen(EMPTY_LN));
    // Editor_SendEscCmd(EMPTY_LN);
    // don't write a newline on the last line, so the terminal doesn't
    //  automatically scroll down another line.
    if (y < e_state.num_rows - 1) {
      Editor_AppendEscCmd(wbuf, NL);
      // Editor_SendEscCmd(NL);
    }
    // WrappedWrite(STDOUT_FILENO, (unsigned char *) (EMPTY_LN NL), strlen(EMPTY_LN NL));
  }
}

static void Editor_ToOrigin(Buffer *wbuf) {
  Editor_AppendEscCmd(wbuf, ESC_MOVE(ORIGIN));
  // Editor_SendEscCmd(ESC_MOVE(ORIGIN));
  // WrappedWrite(STDOUT_FILENO,
  //                     (unsigned char *) ESC_MOVE(ORIGIN),
  //                     strlen(ESC_MOVE(ORIGIN)));
}

static void Editor_AppendEscCmd(Buffer *wbuf, const char *cmd) {
  WriteBuffer_Append(wbuf, (unsigned char *) cmd, strlen(cmd));
}
