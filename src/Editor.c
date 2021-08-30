#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>  // for terminal control
#include <stdio.h>
#include <sys/types.h>  // for ssize_t

#include "Keyboard.h"
#include "TerminalUtils.h"
#include "Quit.h"
#include "WriteBuffer.h"
#include "Editor.h"
#include "ESCCommands.h"
#include "FileIO.h"

// macros

// the size of the welcome message buffer.
#define BUF_SIZE_WEL 64
// the size of an esc cursor move command buffer.
#define BUF_SIZE_MV 32
// the current ctek version.
#define VERSION "1.0"

// in raw mode, terminal does not automatically convert \n to \r\n,
//  so this is used to represent newline characters.
#define NL "\r\n"
// the character that appears on the left of an empty line.
#define EMPTY_LN_CHAR ">"

// converts a character byte to the ctrl version of that key.
// sets highest 3 bits to 0 (i.e., 0x1F == 0b00011111).
#define CHAR_TO_CTRL(key) ((key) & 0x1F)

// state/position of the cursor. 0 indexed.
typedef struct {
  int col;  // horizontal position. x
  int row;  // vertical position. y
} Cursor;

typedef struct {
  int num_rows;
  int num_cols;
} Window;

// global editor state struct.
typedef struct {
  // the original terminal attributes for restoring after
  //  the editor closes.
  struct termios og_term_attr;
  // editor window size.
  int num_rows;
  int num_cols;
  // the state of the cursor.
  Cursor cursor;
  // number of lines of text from the file.
  int num_file_lines;
  // a pointer to an array of FileLines for text lines from the file.
  FileLine *file_lines;
} EditorState;

static EditorState e_state;

// static helper functions.

// draw rows of text.
static void Editor_RenderRows(Buffer *wbuf);
// Append the welcome message to the write buffer.
static void Editor_RenderWelcome(Buffer *wbuf);
// move the cursor in accordance with which key was pressed.
static void Editor_MoveCursor(int key);

void Editor_Open(void) {
  // enable raw mode.
  Term_SetRawMode(&e_state.og_term_attr);
  atexit(Editor_Close);

  // initialize the cursor state to (col,row) = (0,0) (upper left corner).
  e_state.cursor = (Cursor) {0, 0};
  // initialize the number of rows of text to 0.
  e_state.num_rows = 0;
  // set the array of FileLines to NULL
  e_state.file_lines = NULL;

  // get the size of the terminal window.
  int res = Term_Size(&e_state.num_rows, &e_state.num_cols);
  if (res == -1)
    quit("Term_Size");
}

void Editor_Close(void) {
  Term_UnSetRawMode(&e_state.og_term_attr);
}

void Editor_InterpretKeypress(void) {
  int key = Keyboard_ReadKey();

  switch (key) {
    case CHAR_TO_CTRL('q'):
      // recieved quit command (CTRL-Q)
      Editor_Refresh();
      Editor_Close();
      exit(EXIT_SUCCESS);
      break;
    
    case HOME:
    case END:
      e_state.cursor.col = ((key == END) ? (e_state.num_cols - 1) : 0);
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      for (int i = e_state.num_rows; i > 0; i--) {
        Editor_MoveCursor((key == PAGE_UP ? ARROW_UP : ARROW_DOWN));
      }
      break;

    case ARROW_UP:
    case ARROW_RIGHT:
    case ARROW_DOWN:
    case ARROW_LEFT:
      // movement keys.
      Editor_MoveCursor(key);
      break;
  }
}

void Editor_Refresh(void) {
  Buffer write_buf = EMPTY_BUF;

  // hide the cursor while rendering.
  WB_AppendESCCmd(&write_buf,
                  (unsigned char *) ESC_CMD_CUR_MODE(HIDE));
  // move cursor to origin.
  WB_AppendESCCmd(&write_buf,
                  (unsigned char *) ESC_CMD_MOVE(ORIGIN));

  Editor_RenderRows(&write_buf);

  // move the cursor to its current position.
  unsigned char mv_cmd[BUF_SIZE_MV];
  // cursor col and row are indexed from 0, while
  //  screen row and col are 0 indexed.
  Get_ESCCmd_Move(mv_cmd, BUF_SIZE_MV,
                  e_state.cursor.col + 1,
                  e_state.cursor.row + 1);
  WB_AppendESCCmd(&write_buf, mv_cmd);

  // show the cursor.
  WB_AppendESCCmd(&write_buf,
                  (unsigned char *) ESC_CMD_CUR_MODE(SHOW));

  // flush the buffer by writing all commands to stdout.
  WB_Write(&write_buf);
  // free the buffer.
  WB_Free(&write_buf);
}

static void Editor_RenderRows(Buffer *wbuf) {
  for (int y = 0; y < e_state.num_rows; y++) {
    if (y >= e_state.num_file_lines) {
      // row is not part of the text buffer.
      // write the welcome message 1/3rd down the screen.
      if (e_state.num_file_lines == 0 && y == e_state.num_rows / 3) {
        // only show the welcome message when the text buffer is empty.
        Editor_RenderWelcome(wbuf);
      } else {
        WB_AppendESCCmd(wbuf, (unsigned char *) EMPTY_LN_CHAR);
      }
    } else {
      // drawing a row that is part of the text buffer.
      int size = e_state.file_lines[y].size;
      if (size > e_state.num_cols) {
        size = e_state.num_cols;
      }
      WB_Append(wbuf, (unsigned char *) e_state.file_lines[y].line, size);
    }

    // clear the line to the end.
      WB_AppendESCCmd(wbuf, (unsigned char *) ESC_CMD_CLEAR(LINE, END_));
    if (y < e_state.num_rows - 1) {
      WB_AppendESCCmd(wbuf, (unsigned char *) NL);
    }
  }
}

static void Editor_RenderWelcome(Buffer *wbuf) {
  // a buffer for the welcome message
  unsigned char w_msg_buf[BUF_SIZE_WEL];
  int w_len = snprintf((char *) w_msg_buf, BUF_SIZE_WEL,
                       "Ctek: Version %s", VERSION);
  if (w_len > e_state.num_cols) {
    // adjust the welcome string length to fit a smaller screen size.
    w_len = e_state.num_cols;
  }

  // calculate the number of spaces to padd on the left to center the message.
  int margin = (e_state.num_cols - w_len) / 2;
  if (margin > 0) {
    // works because EMPTY_LN_CHAR has a constant length.
    WB_AppendESCCmd(wbuf, (unsigned char *) EMPTY_LN_CHAR);
    margin--;
  }
  while (margin--) {
    WB_AppendESCCmd(wbuf, (unsigned char *) " ");
  }
  WB_Append(wbuf, w_msg_buf, w_len);
}

static void Editor_MoveCursor(int key) {
  // recall, row number increases down, col number increases left.
  // do not change cursor position if the move would bring the cursor
  //  out of bounds on the screen.
  switch (key) {
    case ARROW_UP:
      // move down by 1 row.
      if (e_state.cursor.row != 0) {
        e_state.cursor.row--;
      }
      break;
    case ARROW_RIGHT:
      // move left by 1 column.
      if (e_state.cursor.col != e_state.num_cols - 1) {
        e_state.cursor.col++;
      }
      break;
    case ARROW_DOWN:
      // move up by 1 row.
      if (e_state.cursor.row != e_state.num_rows - 1) {
        e_state.cursor.row++;
      }
      break;
    case ARROW_LEFT:
      // move right by 1 column.
      if (e_state.cursor.col != 0) {
        e_state.cursor.col--;
      }
      break;
  }
}

void Editor_InitFromFile(char *file_name) {
  // read in the lines from the file.
  e_state.file_lines = File_GetLines(file_name, &(e_state.num_file_lines));

  // char *line = File_GetFirstLine(file_name);
  // if (line == NULL) {
  //   quit("File_GetFirstLine");
  // }

  // the line is null-termianted already.
  // ssize_t line_size = strlen(line);

  // e_state.file_lines.size = line_size;
  // e_state.file_lines.line = (char *) malloc(line_size);
  // memcpy(e_state.file_lines.line, line, line_size);
  // // e_state.file_lines.line[line_size] = '\0';
  // e_state.num_file_lines = 1;

  // free(line);
}
