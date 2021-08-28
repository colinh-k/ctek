#include "Quit.h"

void quit(const char *msg) {
  // clear the screen before printing the error and exiting.
  Editor_Refresh();
  // reset the terminal to original state.
  // Editor_Close();

  perror(msg);
  exit(EXIT_FAILURE);
}
