#ifndef FILE_PARSER_H_
#define FILE_PARSER_H_

#include <unistd.h>
#include "SyntaxHL.h"

// struct to store a line of text.
typedef struct {
  // size of the line of characters.
  int size;
  // the size of the line_display string.
  int size_display;
  // pointer to a raw line of characters from a file.
  char *line;
  // the line replaced with characters able to be
  //  appropriately displayed on the terminal window.
  char *line_display;
  // each value in this char array is between 0 and 255
  //  and corresponds to a letter in line_display which
  //  indicates the type of highlighting the character
  //  should get.
  Highlight_t *highlight;
} FileLine;

// TODO: replace with a cursor struct.
typedef struct {
  // the index of the FileLine struct which contains
  //  a match.
  int cur_row;
  // the index into the line field of the FileLine
  //  struct with a match.
  int cur_col;
  // the index into the line_display field of the FileLine
  //  struct with a match.
  // int file_col;// not needed
} SearchResult;


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
FileLine *File_GetLines(const char *file_name, int *size, Syntax *syntax);

void File_FreeLines(FileLine *file_lines, int num_lines);

// converts an index into f_line's line field to an index into the 
//  line_display field. returns the converted cooresponding index.
//  account for tabs in the line that appear as multiple " " 
//  in line_display.
int File_RawToDispIdx(FileLine *f_line, int line_idx);
int File_DispToRawIdx(FileLine *f_line, int disp_idx);

void File_InsertFileLine(FileLine **f_lines, int *num_lines,
                            const char *str, size_t size,
                            int idx, Syntax *syntax);

void File_InsertChar(FileLine *f_line, int idx, char new_char, Syntax *syntax);

// see editor_removechar
void File_RemoveChar(FileLine *f_line, int idx, Syntax *syntax);

// free the malloc'ed buffers in the FileLine. The memory for f_line
//  is unaffected by this function.
void File_FreeFileLineBufs(FileLine *f_line);

// Delete a FileLine at position idx from the given FileLine
//  array containing num_lines FileLines. Decrements num_lines
//  upon successful deletion.
void File_RemoveRow(FileLine *f_line, int *num_lines, int idx);

// Append the given string str of size str_size to the end of f_line's
//  line field.
void File_AppendLine(FileLine *f_line, const char *str, size_t str_size, Syntax *syntax);

void File_SplitLine(FileLine **f_line, int *num_lines, int row, int col, Syntax *syntax);

// Searches the array of FileLines (containing num_lines FileLines) for a
//  line containing str as a substring. Returns 0 on success, -1 on failure.
//  Upon success s_res contains the row and column index into the matching
//  FileLine (see FileParser.h for SearchResult details).
int File_SearchFileLines(FileLine *f_lines, int num_lines, const char *str,
                         SearchResult *s_res);

#endif  // FILE_PARSER_H_
