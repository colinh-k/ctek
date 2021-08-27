#ifndef IOUTILS_H_
#define IOUTILS_H_

#include "Quit.h"
#include "TerminalUtils.h"

// static global variables.

// A struct to save the original terminal attributes.
static struct termios term_attr_og;

// Restores the terminal attributes to their original state before
//  SetRawMode was called. MUST be called only after SetRawMode.
//  Continues trying if recieved EINTR error.
static void UnSetRawMode(void);

static void SetAttr(int fd, int ops, struct termios *attrs);

int SetRawMode(void) {
  // get the current terminal attributes into a global struct.
  // continue trying if tcgetattr recieves an interrupt.
  int ret_val;
  while (1) {
    // get the terminal attributes for standard input.
    ret_val = tcgetattr(STDIN_FILENO, &term_attr_og);
    if (ret_val == -1) {
      // continue;
      if (errno == EINTR) {
        // recoverable error, so try again.
        continue;
      } else {
        // unrecoverable error, so quit.
        quit("SetRawMode [tcgetattr]");
      }
    }
    // no error occurred.
    break;
  }

  // restore original terminal settings at exit.
  atexit(UnSetRawMode);

  // copy the original attributes.
  struct termios term_attr = term_attr_og;

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
  // while (1) {
  //   // TCSAFLUSH: apply after all buffered output is written
  //   //  and ignore unread input.
  //   ret_val = tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attr);
  //   if (ret_val == -1 && errno == EINTR) {
  //     // recoverable error.
  //     continue;
  //   }
  //   // no error.
  //   break;
  // }

  return ret_val;
}

static void UnSetRawMode(void) {
  SetAttr(STDIN_FILENO, TCSAFLUSH, &term_attr_og);
  // int ret_val;
  // while (1) {
  //   // TCSAFLUSH: apply after all buffered output is written
  //   //  and ignore unread input.
  //   ret_val = tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attr_og);
  //   if (ret_val == -1 && errno == EINTR) {
  //     // recoverable error.
  //     continue;
  //   }
  //   // no error.
  //   break;
  // }
}

static void GetAttr() {

}

void SetAttr(int fd, int ops, struct termios *attrs) {
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
ctrl-C == 3
ctrl-Z == 26
ctrl-S == 19
ctrl-Q == 17
ctrl-V == 22
ctrl-V == 22
ctrl-O == 15
ctrl-M, Enter == 13 (carriage return)
*/
