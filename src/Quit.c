#include "Quit.h"

void quit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}
