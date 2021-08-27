#include <termios.h>  // for terminal control
#include <stdlib.h>   // for exit codes, atexit
#include <unistd.h>   // for std fd's
#include <errno.h>    // for error checking

// Returns -1 on error (i.e., the raw mode could not be enablecd),
//  otherwise sets the termios struct for raw character processing
//  and returns 0.
int SetRawMode(void);


// int UnSetRawMode();
