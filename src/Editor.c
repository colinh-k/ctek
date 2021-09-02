#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>  // for terminal control
#include <stdio.h>
#include <sys/types.h>  // for ssize_t

#include <time.h>  // for time_t
#include <stdarg.h>  // for varidic args

#include <unistd.h>  // for exec

#include <stdbool.h>  // for boolean type

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
// the size of the status bar buffer.
#define BUF_SIZE_STATUS 64
// the size of an esc cursor move command buffer.
#define BUF_SIZE_MV 32
// the size of the command message buffer.
#define BUF_SIZE_CMD_MSG 64
// the timeout to display a new message in seconds.
#define MSG_TIMEOUT 5
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

// the string form of a single space.
#define SPACE " "

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
  // the name of the displayed file.
  char *file_name;
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
  // a message to display to the user about commands.
  char msg_line[BUF_SIZE_CMD_MSG];
  // 
  time_t msg_time;
  // true if the editor has changed some text that has not
  //  been saved in the file; false otherwise.
  bool is_edited;
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
// 
static void Editor_RenderStatusBar(Buffer *wbuf);
// returns the smaller of the two numbers.
static int min(int a, int b);
static void Editor_RenderMessageLine(Buffer *wbuf);

static void Editor_InsertChar(char new_char);
static void Editor_Save();
// Removes 1 char from the current line to the left of the cursor. Does
//  nothing if on an empty line, or no char exists to the left of the cursor.
static void Editor_RemoveChar();
// split the current line at the current cursor position.
static void Editor_SplitLine();

void Editor_Open(void) {
  // enable raw mode.
  Term_SetRawMode(&e_state.og_term_attr);
  atexit(Editor_Close);

  // stays NULL if no file passed as an argument to the program.
  e_state.file_name = NULL;
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
  e_state.msg_time = 0;
  // no message to display by default.
  e_state.msg_line[0] = '\0';
  // initially, the editor and file have the same contents.
  e_state.is_edited = false;

  // get the size of the terminal window.
  int res = Term_Size(&e_state.num_rows, &e_state.num_cols);
  if (res == -1)
    quit("Term_Size");
  // reduce number of rows by 2 to make room for a status
  //  bar and message line.
  e_state.num_rows -= 2;
}

void Editor_Close(void) {
  // restore the terminal to its original state.
  Term_UnSetRawMode(&e_state.og_term_attr);
  // free malloc'ed array of file lines.
  File_FreeLines((e_state.file_lines), e_state.num_file_lines);
  // no error checking with quit, since that might
  //  start a loop of error catching.
  // clear the screen.
  write(STDOUT_FILENO, ESC_CMD_CLEAR(SCREEN, ALL),
        sizeof(ESC_CMD_CLEAR(SCREEN, ALL)));
  // move the cursor to the origin.
  write(STDOUT_FILENO, ESC_CMD_MOVE(ORIGIN),
        sizeof(ESC_CMD_MOVE(ORIGIN)));
  // char *str_clear = "clear";
  // char **args = &str_clear;
  // execvp("clear", args);
}

void Editor_InterpretKeypress(void) {
  // static bool pressed_force = false;
  static bool pressed_quit = false;
  int key = Keyboard_ReadKey();

  switch (key) {
    case CHAR_TO_CTRL('q'):
      // recieved quit command (CTRL-Q).
      // ensure user wants to discard unsaved changes.
      if (e_state.is_edited) {
        Editor_SetCmdMsg("WARN: unsaved changes. Type '!' to force.");
        pressed_quit = true;
        return;
      }
      // no error checking with quit, since that might
      //  start a loop of error catching.
      // clear the screen.
      // write(STDOUT_FILENO, ESC_CMD_CLEAR(SCREEN, ALL),
      //       sizeof(ESC_CMD_CLEAR(SCREEN, ALL)));
      // // move the cursor to the origin.
      // write(STDOUT_FILENO, ESC_CMD_MOVE(ORIGIN),
      //       sizeof(ESC_CMD_MOVE(ORIGIN)));
      exit(EXIT_SUCCESS);
      break;

    case KEY_RETURN:
      // split the line at the cursor's current location.
      fprintf(stderr, "ENTER PRESSED\n");
      Editor_SplitLine();
      break;
    
    case KEY_DELETE:
      // the delete key keeps the cursor in place, so move 1 right to
      //  counteract the deletion of the char to the left. Effectively,
      //  the delete key removes the char highlighted by the cursor.
      // TODO: fix weird effect when DEL is pressed on the end of a line.
      Editor_MoveCursor(KEY_ARROW_RIGHT);
    case KEY_BACKSPACE:
    case CHAR_TO_CTRL('h'):
      Editor_RemoveChar();
      break;

    case CHAR_TO_CTRL('l'):
    case KEY_ESC:
      break;

    case CHAR_TO_CTRL('s'):
      // save command
      Editor_Save();
      break;
    
    case KEY_HOME:
      e_state.cursor.col = 0;
      break;

    case KEY_END:
      // snap to the end of a line.
      if (e_state.cursor.row < e_state.num_file_lines) {
        e_state.cursor.col = e_state.file_lines[e_state.cursor.row].size;
      }
      break;

    case KEY_PAGE_UP:
    case KEY_PAGE_DOWN:
      // scroll up or down by snapping cursor to either the top or bottom of
      //  the window, then moving the page down with Editor_MoveCursor.
      if (key == KEY_PAGE_UP) {
        e_state.cursor.row = e_state.cur_file_row;
      } else {
        e_state.cursor.row = e_state.cur_file_row + e_state.num_rows - 1;
        if (e_state.cursor.row > e_state.num_file_lines) {
          // prevent out of bounds jump beyond bottom of screen.
          e_state.cursor.row = e_state.num_file_lines;
        }
      }
      for (int i = e_state.num_rows; i > 0; i--) {
        Editor_MoveCursor((key == KEY_PAGE_UP ? KEY_ARROW_UP : KEY_ARROW_DOWN));
      }
      break;

    case KEY_ARROW_UP:
    case KEY_ARROW_RIGHT:
    case KEY_ARROW_DOWN:
    case KEY_ARROW_LEFT:
      // movement keys.
      Editor_MoveCursor(key);
      break;

    case '!':
      if (e_state.is_edited && pressed_quit) {
        exit(EXIT_SUCCESS);
      }
    default:
      // any key that is not an editor key binding is inserted into
      //  the line.
      Editor_InsertChar(key);
      break;
  }

  // reset the quit sequence indicators by default.
  // pressed_force = false;
  pressed_quit = false;
}

void Editor_Refresh(void) {
  Editor_Scroll();

  Buffer write_buf = EMPTY_BUF;

  // hide the cursor while rendering.
  WB_AppendESCCmd(&write_buf,
                  ESC_CMD_CUR_MODE(HIDE));
  // move cursor to origin.
  WB_AppendESCCmd(&write_buf,
                  ESC_CMD_MOVE(ORIGIN));

  Editor_RenderRows(&write_buf);
  Editor_RenderStatusBar(&write_buf);
  Editor_RenderMessageLine(&write_buf);

  // move the cursor to its current position.
  char mv_cmd[BUF_SIZE_MV];
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
                  ESC_CMD_CUR_MODE(SHOW));

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
        WB_AppendESCCmd(wbuf, EMPTY_LN_CHAR);
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
      WB_Append(wbuf,                 (&(e_state.file_lines[disp_line].line_display[e_state.cur_file_col])),
                size);
    }

    // clear the line to the end.
    WB_AppendESCCmd(wbuf, ESC_CMD_CLEAR(LINE, END_));
    // if (y < e_state.num_rows - 1) {
      WB_AppendESCCmd(wbuf, NL);
    // }
  }
}

static void Editor_RenderWelcome(Buffer *wbuf) {
  // a buffer for the welcome message
  char w_msg_buf[BUF_SIZE_WEL];
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
    WB_AppendESCCmd(wbuf, EMPTY_LN_CHAR);
    margin--;
  }
  while (margin--) {
    WB_AppendESCCmd(wbuf, " ");
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
    case KEY_ARROW_UP:
      // move down by 1 row.
      if (e_state.cursor.row != 0) {
        e_state.cursor.row--;
      }
      break;
    case KEY_ARROW_RIGHT:
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
    case KEY_ARROW_DOWN:
      // increment row (going down)
      // allow the cursor to go past the bottom of the screen,
      //  but not past the bottom of the file.
      if (e_state.cursor.row < e_state.num_file_lines) {
        e_state.cursor.row++;
      }
      break;
    case KEY_ARROW_LEFT:
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
  // set the file name in the global struct.
  free(e_state.file_name);
  // strdup malloc's memory for the string copy.
  e_state.file_name = strdup(file_name);
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

static void Editor_RenderStatusBar(Buffer *wbuf) {
  WB_AppendESCCmd(wbuf, ESC_CMD_TEXT_FORMAT(INVERT));

  // build the status string in two parts; one left-aligned,
  //  the other right-aligned.
  char status_line_left[BUF_SIZE_STATUS],
       status_line_right[BUF_SIZE_STATUS];
  // shows "<NEW FILE>" if no file name was given.
  char *file_name = (e_state.file_name != NULL) ?
                    e_state.file_name : "<NEW FILE>";
  char *mod_status = (e_state.is_edited) ? "| <modified>" : "";
  // shows at most 20 characters from the file name.
  int status_size_left = snprintf(status_line_left, BUF_SIZE_STATUS,
                             "%d lines from %.20s %s",
                             e_state.num_file_lines,
                             file_name,
                             mod_status);
  if (status_size_left > e_state.num_cols) {
    // the status string is too wide to fit in the screen,
    //  so set its size to the maximum it can be on the screen.
    status_size_left = e_state.num_cols;
  }

  // print the current line number out of total lines.
  // e_state.cursor.row is 0 indexed, so add 1 to the displayed value.
  int status_size_right = snprintf(status_line_right, BUF_SIZE_STATUS,
                                   "<%d> | <%d>",
                                   e_state.cursor.row + 1,
                                   e_state.num_file_lines);

  // write the left side of the status bar to the buffer.
  WB_Append(wbuf, status_line_left, status_size_left);

  // write spaces to the buffer until adding the right side would
  //  align it flush with the last column on the right.
  while (status_size_left < e_state.num_cols) {
    if (e_state.num_cols - status_size_left == status_size_right) {
      // if there is exactly enough space for the right side,
      //  write it to the buffer.
      WB_Append(wbuf, status_line_right, status_size_right);
      break;
    }
    // draw spaces to the edge of the screen so the status
    //  is on an inverted background.
    WB_AppendESCCmd(wbuf, SPACE);
    status_size_left++;
  }

  WB_AppendESCCmd(wbuf, ESC_CMD_TEXT_FORMAT(DEFAULT));
  // make room for the message line.
  WB_AppendESCCmd(wbuf, NL);
}

void Editor_SetCmdMsg(const char *msg, ...) {
  // setup varidic args.
  va_list args;
  va_start(args, msg);

  // expand the format message with the given args, and
  //  copy the message into the global struct.
  vsnprintf(e_state.msg_line, BUF_SIZE_CMD_MSG, msg, args);
  va_end(args);
  // NULL arg gets current time.
  e_state.msg_time = time(NULL);
}

static void Editor_RenderMessageLine(Buffer *wbuf) {
  // clear the line.
  WB_AppendESCCmd(wbuf, ESC_CMD_CLEAR(LINE, END_));
  // ensure the message can fit in the window.
  int msg_size = min(strlen(e_state.msg_line), e_state.num_cols);
  // only render the message if it is less than MSG_TIMEOUT seconds old.
  if (msg_size != 0 && time(NULL) - e_state.msg_time < MSG_TIMEOUT) {
    WB_Append(wbuf, e_state.msg_line, msg_size);
  }
}

// TODO: use min when deciding if a message can be displayed on the screen.
static int min(int a, int b) {
  return (a > b) ? b : a;
}

static void Editor_InsertChar(char new_char) {
  if (e_state.cursor.row == e_state.num_file_lines) {
    // if the cursor is on the last line, append a new FileLine to the
    //  array of file lines.
    File_InsertFileLine(&(e_state.file_lines),
                        &(e_state.num_file_lines), "", 0,
                        e_state.num_file_lines);
  }
  File_InsertChar(&(e_state.file_lines[e_state.cursor.row]),
                  e_state.cursor.col, new_char);
  // move the cursor 1 column to the right so the next character inserted
  //  is on a different space.
  e_state.cursor.col++;
  // record that the file was edited.
  e_state.is_edited = true;
}

// Removes 1 char from the current line to the left of the cursor. Does
//  nothing if on an empty line, or no char exists to the left of the cursor.
static void Editor_RemoveChar() {
  // TODO: combine conditions.
  if (e_state.cursor.row == e_state.num_file_lines ||
      (e_state.cursor.col == 0 && e_state.cursor.row == 0)) {
    // on an empty line, so return early, or on the first line, first
    //  column in which case there is also nothing to do.
    return;
  }

  if (e_state.cursor.col > 0) {
    // on a line with a char to the left of the cursor,
    //  so delete it.
    File_RemoveChar(&(e_state.file_lines[e_state.cursor.row]),
                    e_state.cursor.col - 1);
    // move the cursor back by 1 column.
    e_state.cursor.col--;
    // record that the file has been changed in the editor.
    e_state.is_edited = true;
  } else {
    // at the start of a line, so append this line to the one above it,
    //  and delete the current line from the array of FileLines.
    // the cursor's new position is 1 line above, at the last column on the line.
    e_state.cursor.col = e_state.file_lines[e_state.cursor.row - 1].size;
    File_AppendLine(&(e_state.file_lines[e_state.cursor.row - 1]),
                    e_state.file_lines[e_state.cursor.row].line,
                    e_state.file_lines[e_state.cursor.row].size);
    File_RemoveRow(e_state.file_lines, &(e_state.num_file_lines), e_state.cursor.row);
    e_state.cursor.row--;
  }
}

static void Editor_Save() {
  int res = File_Save(e_state.file_name, &(e_state.file_lines),
                e_state.num_file_lines);

  if (res == -1) {
    // error saving.
    Editor_SetCmdMsg("ERROR: file NOT saved: %s", strerror(errno));
  } else {
    Editor_SetCmdMsg("SAVE SUCCESSFUL: %d bytes written to %s",
                     res, e_state.file_name);
    // record that the editor and file are in sync.
    e_state.is_edited = false;
  }
}

static void Editor_SplitLine() {
  File_SplitLine(&(e_state.file_lines), &(e_state.num_file_lines),
                 e_state.cursor.row, e_state.cursor.col);
  // set the cursor to one line down and to the start of the line.
  e_state.cursor.row++;
  e_state.cursor.col = 0;
}
