#include <stdlib.h>  // for exit codes, exit
#include <unistd.h>  // for POSIX api
#include <ctype.h>  // for iscntrl (is control character)
#include <stdio.h>  // for perror

#include "Editor.h"
#include "Quit.h"

// static helper functions.

int main(int argc, char const *argv[]) {
  Editor_Open();
  if (argc == 2) {
    // if passed a filename, initialize the editor with the file.
    Editor_InitFromFile(argv[1]);
  }

  while (1) {
    Editor_Refresh();
    Editor_InterpretKeypress();
  }
  return EXIT_SUCCESS;
}
