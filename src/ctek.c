#include <stdlib.h>  // for exit codes, exit
#include <unistd.h>  // for POSIX api
#include <ctype.h>  // for iscntrl (is control character)
#include <stdio.h>  // for perror

#include "TerminalUtils.h"
#include "IOUtils.h"
#include "Quit.h"

// in raw mode, terminal does not automatically convert \n to \r\n,
//  so this is used to represent newline characters.
#define NL "\r\n"

// static helper functions.

int main(int argc, char const *argv[]) {
  SetRawMode();

  while (1) {
    unsigned char c = '\0';
    if (ReadFD(STDIN_FILENO, &c, 1) && errno != EAGAIN) 
      quit("ReadFD");
    if (iscntrl(c))
      printf("%d" NL, c);
    else
      printf("%d ('%c')" NL, c, c);
    if (c == 'q') break;
  }
  return EXIT_SUCCESS;
}
