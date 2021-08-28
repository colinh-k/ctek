#include <stdlib.h>  // for exit codes, exit
#include <unistd.h>  // for POSIX api
#include <ctype.h>  // for iscntrl (is control character)
#include <stdio.h>  // for perror

// #include "TerminalUtils.h"
#include "Editor.h"
#include "Quit.h"

// static helper functions.

int main(int argc, char const *argv[]) {
  Editor_Open();

  while (1) {
    Editor_Refresh();
    Editor_InterpretKeypress();
  }

  Editor_Close();
  return EXIT_SUCCESS;
}
