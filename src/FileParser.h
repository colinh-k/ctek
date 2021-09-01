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


// const unsigned char *File_ToString(FileLine **file_lines, int num_lines,
//                     size_t *file_size);

// Save the given file_lines (an array of size num_lines)
//  in a file named file_name. Creates and writes to
//  a new file if the name does not exist as a file.
//  Returns the number of bytes written to the file.
int File_Save(const char *file_name, FileLine **file_lines,
               int num_lines);

// Returns a malloc'ed array of FileLines from the given file, delimiting on
//  \n and \r. Client must call File_FreeLines later.
FileLine *File_GetLines(const char *file_name, int *size);

void File_FreeLines(FileLine *file_lines, int num_lines);

// converts an index into f_line's line field to an index into the 
//  line_display field. returns the converted cooresponding index.
//  account for tabs in the line that appear as multiple " " 
//  in line_display.
int File_RawToDispIdx(FileLine *f_line, int line_idx);

void File_AppendFileLine(FileLine **f_lines, int *num_lines,
                            const char *str, size_t size);

void File_InsertChar(FileLine *f_line, int idx, char new_char);

// see editor_removechar
void File_RemoveChar(FileLine *f_line, int idx);

// free the malloc'ed buffers in the FileLine. The memory for f_line
//  is unaffected by this function.
void File_FreeFileLineBufs(FileLine *f_line);

// Delete a FileLine at position idx from the given FileLine
//  array containing num_lines FileLines. Decrements num_lines
//  upon successful deletion.
void File_RemoveRow(FileLine *f_line, int *num_lines, int idx);

// Append the given string str of size str_size to the end of f_line's
//  line field.
void File_AppendLine(FileLine *f_line, const char *str, size_t str_size);

#endif  // FILE_PARSER_H_
