// #include <stdlib.h>
#include <errno.h>
// #include <stdio.h>
#include <unistd.h>  // for POSIX read

// Reads in at most num_chars bytes into the given buffer and returns the
//  number of bytes read. Returns 0 if EOF is reached or read times out
//  and no bytes have been read. Returns -1 on fatal error. The buffer
//  MUST have enough space for at least num_chars bytes.
int ReadFD(int fd, unsigned char *buf, int num_chars);
