#include <stdlib.h>  // for exit codes, exit
#include <unistd.h>  // for POSIX api
#include <ctype.h>  // for iscntrl (is control character)
#include <stdio.h>  // for perror

#include "Editor.h"
#include "Quit.h"

// static helper functions.

int main(int argc, char const *argv[]) {
  Editor_Open();
  if (argc > 2) {
    fprintf(stderr, "Please pass 1 filename, or none to begin a new file");
    return EXIT_SUCCESS;
  }
  if (argc == 2) {
    // if passed a filename, initialize the editor with the file.
    Editor_InitFromFile(argv[1]);
  }

  Editor_SetCmdMsg("USAGE: CTRL-Q to quit | CTRL-S to save to file");

  while (1) {
    Editor_Refresh();
    Editor_InterpretKeypress();
  }
  return EXIT_SUCCESS;
}
