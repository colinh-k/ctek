#ifndef FILE_PARSER_H_
#define FILE_PARSER_H_

#include <unistd.h>

// struct to store a line of text.
typedef struct {
  // size of the line of characters.
  int size;
  int size_display;
  // pointer to a raw line of characters from a file.
  char *line;
  // the line replaced with characters able to be
  //  appropriately displayed on the terminal window.
  char *line_display;
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

void File_FreeLines(FileLine *file_lines, int num_lines);

// converts an index into f_line's line field to an index into the 
//  line_display field. returns the converted cooresponding index.
//  account for tabs in the line that appear as multiple " " 
//  in line_display.
int File_RawToDispIdx(FileLine *f_line, int line_idx);

#endif  // FILE_PARSER_H_
