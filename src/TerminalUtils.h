#include <termios.h>  // for terminal control
#include <stdlib.h>   // for exit codes, atexit
#include <unistd.h>   // for std fd's
#include <errno.h>    // for error checking
#include <sys/ioctl.h>  // for ioctl, winsize

#include <ctype.h>

#include "Quit.h"

// Saves the original termios struct in the given location
//  before setting the terminal to raw mode.
void Term_SetRawMode(struct termios *og_termios);

// Restores the terminal attributes to the state supplied by
//  og_termios. Continues trying if recieved EINTR error.
void Term_UnSetRawMode(struct termios *og_termios);

// Get the size of the window.
int Term_Size(int *rows, int *cols);
