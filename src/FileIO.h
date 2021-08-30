#ifndef File_IO_H_
#define File_IO_H_

#include <unistd.h>

// struct to store a line of text.
typedef struct {
  // size of the line of characters.
  int size;
  // pointer to a line of characters.
  char *line;
} FileLine;

// Returns a buffer containing the contents of a file named file_name.
//  If no such file exists (or insufficient permissions), returns NULL.
//  Returns NULL if reading failed. Upon successful return, size
//  holds the number of bytes read by the file. The client is responsible
//  for free'ing the returned pointer.
char *File_ToString(const char *file_name, int *size);

// Returns a malloc'ed array of FileLines from the given file, delimiting on
//  \n and \r. Client must call File_FreeLines later.
FileLine *File_GetLines(const char *file_name, int *size);

void File_FreeLines(FileLine **file_lines);

#endif  // File_IO_H_
