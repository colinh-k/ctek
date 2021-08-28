#ifndef IOUTILS_H_
#define IOUTILS_H_

#include "TerminalUtils.h"

// static global variables.

// A struct to save the original terminal attributes.
// static struct termios term_attr_og;

static void GetAttr(struct termios *attrs);
static void SetAttr(int fd, int ops, struct termios *attrs);
static int Term_CurorPosition(int *row, int *col);

void Term_SetRawMode(struct termios *og_termios) {
  // get the current terminal attributes into a global struct.
  GetAttr(og_termios);
  // continue trying if tcgetattr recieves an interrupt.

  // restore original terminal settings at exit.
  // atexit(Term_UnSetRawMode);

  // copy the original attributes.
  struct termios term_attr = *og_termios;
  // modify terminal attributes.
  // c_lflag: set local flag
  // remove automatic echo and process characters immediately.
  // disable ctl-C, ctl-Z (and ctl-Y, ctol-O on macOS), ctl-V signals.
  term_attr.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  // disable ctl-S, ctl-Q, ctl-M 
  term_attr.c_iflag &= ~(IXON | ICRNL | INPCK | ISTRIP | BRKINT);
  // disable output processing.
  term_attr.c_oflag &= ~(OPOST);
  // with BRKINT, INPCK, ISTRIP, originally used to set things
  //  on old terminals; probably unecessary now.
  term_attr.c_cflag |= (CS8);
  // set timeout for POSIX read to return.
  // min input bytes for read to return.
  term_attr.c_cc[VMIN] = 0;
  // max time to wait for input before read returns.
  // units are 1/10 sec.
  term_attr.c_cc[VTIME] = 1;

  // set the termios settings.
  SetAttr(STDIN_FILENO, TCSAFLUSH, &term_attr);
}

void Term_UnSetRawMode(struct termios *og_termios) {
  SetAttr(STDIN_FILENO, TCSAFLUSH, og_termios);
}

int Term_Size(int *rows, int *cols) {
  struct winsize window_size;

  int res = ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
  if (res == -1 || window_size.ws_col == 0) {
    // on ioctl failure, it is possible to still get the window
    //  size by moveing the cursor to the bottom right and getting
    //  its position with escape sequences.
    // 999C: move right by 999
    // 999B: move down by 999
    int res = WrappedWrite(STDOUT_FILENO, (unsigned char *) "\x1b[999C\x1b[999B", 12);
    if (res != 12) return -1;
    return Term_CurorPosition(rows, cols);
  }
  else {
    *cols = window_size.ws_col;
    *rows = window_size.ws_row;
    return 0;
  }
}

static int Term_CurorPosition(int *row, int *col) {
  // we get the result of an n escape sequence through stdin
  //  and store it in a buffer. result form: "<esc>[<col>;<row>R"
  unsigned char buf[32];
  unsigned int i = 0;

  // 6n: get cursor position
  int res = WrappedWrite(STDOUT_FILENO, (unsigned char *) "\x1b[6n", 4);
  if (res != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (WrappedRead(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  // null terminate the response string.
  buf[i] = '\0';
  // parse the response. expect an ESC character and a '[' in the first two
  //  positions.
  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf((const char *) &buf[2], "%d;%d", row, col) != 2)
    return -1;
  return 0;
}

// static helper function definitions.

static void GetAttr(struct termios *attrs) {
  while (1) {
    // get the terminal attributes for standard input.
    int ret_val = tcgetattr(STDIN_FILENO, attrs);
    if (ret_val == -1) {
      // continue;
      if (errno == EINTR) {
        // recoverable error, so try again.
        continue;
      } else {
        // unrecoverable error, so quit.
        quit("Term_SetRawMode [tcgetattr]");
      }
    }
    // no error occurred.
    break;
  }
}

static void SetAttr(int fd, int ops, struct termios *attrs) {
  while (1) {
    // TCSAFLUSH: apply after all buffered output is written
    //  and ignore unread input.
    int ret_val = tcsetattr(fd, ops, attrs);
    if (ret_val == -1) {
      if (errno == EINTR) {
        // recoverable error.
        continue;
      }
      // unrecoverable error.
      quit("SetAttr");
    }
    // no error.
    break;
  }
}

#endif  // IOUTILS_H_

/*
ctrl-C == 3s
ctrl-Z == 26
ctrl-S == 19
ctrl-Q == 17
ctrl-V == 22
ctrl-V == 22
ctrl-O == 15
ctrl-M, Enter == 13 (carriage return)
\x1b == 27 (escape character)
*/
