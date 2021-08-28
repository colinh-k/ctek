#include <errno.h>
#include <unistd.h>  // for POSIX read

#include "Quit.h"

// Reads in at most num_chars bytes into the given buffer and returns the
//  number of bytes read. Returns 0 if EOF is reached or read times out
//  and no bytes have been read. Returns -1 on fatal error. The buffer
//  MUST have enough space for at least num_chars bytes.
int WrappedRead(int fd, unsigned char *buf, int num_chars);

// Writes num_bytes bytes to the given file from the given buffer. Returns
//  if an error is encountered or num_bytes bytes have been written. Returns
//  the number of written bytes. A fatal error is indicated if the return 
//  value is less than num_bytes.
int WrappedWrite(int fd, const unsigned char *buf, int num_bytes);

// Reads and returns 1 keypress from stdin. Calls quit if WrappedRead fails.
char ReadKey(void);
