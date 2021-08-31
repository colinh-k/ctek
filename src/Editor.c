#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>  // for terminal control
#include <stdio.h>
#include <sys/types.h>  // for ssize_t

#include "Editor.h"
#include "TerminalUtils.h"
#include "Keyboard.h"
#include "WriteBuffer.h"
#include "ESCCommands.h"
#include "FileParser.h"
#include "Quit.h"

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
  // with cur_file_*, num_rows now refers to cursor position in the file.
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
  // the current line number of the file being shown
  //  (indexed from top of file).
  int cur_file_row;
  // current column offset into the file.
  int cur_file_col;
  // a pointer to an array of FileLines for text lines from the file.
  FileLine *file_lines;
  // an index into the lind_display of the current FileLine.
  int ld_idx;
} EditorState;

static EditorState e_state;

// static helper functions.

// draw rows of text.
static void Editor_RenderRows(Buffer *wbuf);
// Append the welcome message to the write buffer.
static void Editor_RenderWelcome(Buffer *wbuf);
// move the cursor in accordance with which key was pressed.
static void Editor_MoveCursor(int key);
// adjust the cur_file_row according to the new cursor location.
static void Editor_Scroll(void);

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
  // set the current line to 0.
  e_state.cur_file_row = 0;
  e_state.cur_file_col = 0;
  e_state.ld_idx = 0;

  // get the size of the terminal window.
  int res = Term_Size(&e_state.num_rows, &e_state.num_cols);
  if (res == -1)
    quit("Term_Size");
}

void Editor_Close(void) {
  // restore the terminal to its original state.
  Term_UnSetRawMode(&e_state.og_term_attr);
  // free malloc'ed array of file lines.
  File_FreeLines((e_state.file_lines), e_state.num_file_lines);
}

void Editor_InterpretKeypress(void) {
  int key = Keyboard_ReadKey();

  switch (key) {
    case CHAR_TO_CTRL('q'):
      // recieved quit command (CTRL-Q)
      Editor_Refresh();
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
  Editor_Scroll();

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
  //  screen row and col are 1 indexed. (TODO: is this correct?)
  // set the cursor position in the line according to the current
  //  position in the line_display field (ld_idx).
  Get_ESCCmd_Move(mv_cmd, BUF_SIZE_MV,
                  (e_state.ld_idx - e_state.cur_file_col) + 1,
                  (e_state.cursor.row - e_state.cur_file_row) + 1);
  WB_AppendESCCmd(&write_buf, mv_cmd);

  // show the cursor.
  WB_AppendESCCmd(&write_buf,
                  (unsigned char *) ESC_CMD_CUR_MODE(SHOW));

  // write all commands to stdout.
  WB_Write(&write_buf);
  // free the buffer.
  WB_Free(&write_buf);
}

static void Editor_RenderRows(Buffer *wbuf) {
  for (int y = 0; y < e_state.num_rows; y++) {
    // calculate the file line to display on the current screen row.
    int disp_line = y + e_state.cur_file_row;
    if (disp_line >= e_state.num_file_lines) {
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
      // calculate how much of the row to show by subtracting the 
      //  column position in the file from the size of the line.
      int size = e_state.file_lines[disp_line].size_display - e_state.cur_file_col;
      if (size < 0) {
        // scrolled to far over to view any chars from this line.
        size = 0;
      }
      if (size > e_state.num_cols) {
        size = e_state.num_cols;
      }
      WB_Append(wbuf, (unsigned char *)
                (&(e_state.file_lines[disp_line].line_display[e_state.cur_file_col])),
                size);
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
  // if the current cursor row position is greater than the number of
  //  lines in the file, set line to NULL. Otherwise, set line to point
  //  to the last line in the file.
  FileLine *line = (e_state.cursor.row >= e_state.num_file_lines) ?
                    NULL : &(e_state.file_lines[e_state.cursor.row]);
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
      // allowed to scroll past right of screen.
      // check that the cursor column is to the left of the
      //  end of the line to prevent scrolling too far right.
      if (line != NULL && e_state.cursor.col < line->size) {
        e_state.cursor.col++;
      } else if (line != NULL && e_state.cursor.col == line->size) {
        // moving right at the end of a line puts cursor on next
        //  line below at the start of the line.
        e_state.cursor.row++;
        e_state.cursor.col = 0;
      }
      break;
    case ARROW_DOWN:
      // increment row (going down)
      // allow the cursor to go past the bottom of the screen,
      //  but not past the bottom of the file.
      if (e_state.cursor.row < e_state.num_file_lines) {
        e_state.cursor.row++;
      }
      break;
    case ARROW_LEFT:
      // move right by 1 column.
      if (e_state.cursor.col != 0) {
        e_state.cursor.col--;
      } else if (e_state.cursor.row > 0) {
        // move to the end of upper adjacent line if moving left of
        //  the viewable window.
        e_state.cursor.row--;
        e_state.cursor.col = e_state.file_lines[e_state.cursor.row].size;
      }
      break;
  }

  line = (e_state.cursor.row >= e_state.num_file_lines) ?
          NULL : &(e_state.file_lines[e_state.cursor.row]);
  int line_size = (line != NULL) ? line->size : 0;
  if (e_state.cursor.col > line_size) {
    // adjust the cursor horizontally to the end of a shorter
    //  line, if previously on a longer line.
    e_state.cursor.col = line_size;
  }
}

void Editor_InitFromFile(const char *file_name) {
  // read in the lines from the file.
  e_state.file_lines = File_GetLines(file_name, &(e_state.num_file_lines));
}

static void Editor_Scroll(void) {
  e_state.ld_idx = e_state.cursor.col;
  // if (e_state.cursor.row < e_state.num_file_lines) {
  //   e_state.ld_idx =
  //   File_RawToDispIdx(&(e_state.file_lines[e_state.cursor.row]), e_state.cursor.col);
  // }

  // vertical scroll correction.
  // check if cursor is above visible screen, and scrolls up to
  //  the cursor location if true.
  if (e_state.cursor.row < e_state.cur_file_row) {
    e_state.cur_file_row = e_state.cursor.row;
  }
  // check if cursor is below visible screen, and adjust to cursor location.
  if (e_state.cursor.row >= e_state.cur_file_row + e_state.num_rows) {
    e_state.cur_file_row = e_state.cursor.row - e_state.num_rows + 1;
  }

  // horizontal scroll correction.
  if (e_state.ld_idx < e_state.cur_file_col) {
    e_state.cur_file_col = e_state.ld_idx;
  }
  if (e_state.ld_idx >= e_state.cur_file_col + e_state.num_cols) {
    e_state.cur_file_col = e_state.ld_idx - e_state.num_cols + 1;
  }
}
