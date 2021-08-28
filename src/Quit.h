#ifndef QUIT_H_
#define QUIT_H_

#include <stdio.h>
#include <stdlib.h>

#include "Editor.h"  // for clearing screen, moving cursor

// Prints the message with perror and exits with EXIT_FAILURE.
void quit(const char *msg);

#endif  // QUIT_H_
