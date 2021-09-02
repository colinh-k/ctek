#include "FileParser.h"
#include "IOUtils.h"
#include "Quit.h"

#include <sys/stat.h>  // for struct stat
#include <fcntl.h>
#include <stdlib.h>  // for NULL
#include <string.h>  // for strchr
#include <unistd.h>  // for close
#include <stdio.h> 

// the size of a single tab character in number of spaces (" ").
#define TAB_SIZE 8
// a single tab character.
#define TAB '\t'
// a single space character.
#define SPACE_CHAR ' '
// the default permissions for a text file. (user: rw; others: r)
#define PERMS_DEFAULT 0644

// static int File_Open(const char *file_name, int *fd, int *size);
static int validate_idx(int idx, int size);

// Returns a pointer to a string containing all the lines from the given
//  FileLines array containing num_lines FileLines. The caller is 
//  responsible for free'ing the returned pointer. Upon return,
//  file_size contains the size of the allocated buffer/string.
const unsigned char *File_ToString(FileLine **file_lines, int num_lines,
                    int *file_size) {
  for (int i = 0; i < num_lines; i++) {
    // calculate the total number of bytes per line (plus 1 for newline
    //  characters for each line). set the total in the output parameter.
    *file_size += (*file_lines)[i].size + 1;
  }

  // malloc space for the string.
  char *buf = (char *) malloc(*file_size);
  if (buf == NULL) {
    // malloc failed.
    return NULL;
  }
  // an index into the string buffer.
  char *buf_idx = buf;

  for (int i = 0; i < num_lines; i++) {
    // copy the line into the buffer at the apprpriate lcoation.
    memcpy(buf_idx, (*file_lines)[i].line, (*file_lines)[i].size);
    // move the buffer index pointer ahead of the copied line.
    buf_idx += (*file_lines)[i].size;
    // set the end of the line with a newline character.
    *buf_idx = '\n';
    // increment the buffer index.
    buf_idx++;
  }

  return (const unsigned char *) buf;
}

int File_Save(const char *file_name, FileLine **file_lines,
               int num_lines) {
  // TODO: write to a temporary file, then rename the file to avoid
  //  issues with truncating.
  if (file_name == NULL) {
    return -1;
  }

  int file_size = 0;
  const unsigned char *file_str = File_ToString(file_lines, num_lines, &file_size);
  // O_RDWR: open for reading and writing.
  // O_CREAT: create the file with the given name if it doesn't exist.
  // 0644: default permissions for a newly created file (user: rw; others: r)
  int fd = open(file_name, O_RDWR | O_CREAT, PERMS_DEFAULT);
  if (fd == -1) {
    // open error.
    free((void *) file_str);
    return -1;
  }

  // make the file have a size of file_size, since the new file_str
  //  might contain fewer characters than was already in the file,
  //  so discard whatever was not overwritten. also, pad with \0
  //  bytes if the file is shorter than file_size.
  if (ftruncate(fd, file_size) == -1) {
    // ftruncate error.
    close(fd);
    free((void *) file_str);
    return -1;
  }

  if (WrappedWrite(fd, file_str, file_size) != file_size) {
    // write error. clean up regardless.
  }

  close(fd);
  free((void *) file_str);
  return file_size;
}

static int StrCount(const char *str, int size, char target) {
  int count = 0;
  for (int i = 0; i < size; i++) {
    if (str[i] == target) {
      count++;
    }
  }
  return count;
}

// Copies the file_line's line into its line_display and replaces
//  all non-renderable characters (like tabs) with appropriate
//  substitutes (like " " (spaces) for tabs).
static void File_SetLineDisplay(FileLine *file_line) {
  // find how much memory to allocate for tab conversion.
  int num_tabs = StrCount(file_line->line, file_line->size, TAB);

  free(file_line->line_display);
  // TODO: use realloc here instead of free and malloc ?
  file_line->line_display = malloc(file_line->size + (num_tabs * (TAB_SIZE - 1)) + 1);

  // after loop, idx_line_disp contains num chars copied to line_display.
  int idx_line_disp = 0;
  for (int idx_line = 0; idx_line < file_line->size; idx_line++) {
    if (file_line->line[idx_line] == TAB) {
      // when replacing tabs, always insert 1 space, then check and
      //  add more so the line reaches the nearest column number
      //  divisible by TAB_SIZE.
      file_line->line_display[idx_line_disp++] = SPACE_CHAR;
      while (idx_line_disp % TAB_SIZE != 0) {
        file_line->line_display[idx_line_disp++] = SPACE_CHAR;
      }
    } else {
      file_line->line_display[idx_line_disp++] = file_line->line[idx_line];
    }
  }
  file_line->line_display[idx_line_disp] = '\0';
  file_line->size_display = idx_line_disp;
}

// Insert the given string 'str' with the given size 'size'
//  to the given array of FileLines as a new FileLine struct
//  at position idx.
//  'num_lines' is the number of FileLine objects in 'f_lines'.
//  'num_lines' is incremented by 1 after the operation.
void File_InsertFileLine(FileLine **f_lines, int *num_lines,
                            const char *str, size_t size,
                            int idx) {
  if (idx < 0 || idx > *num_lines) {
    return;
  }

  // increase the size of the array by an extra sizeof(FileLine).
  *f_lines = realloc(*f_lines, (*num_lines + 1) * sizeof(FileLine));
  // make room for the new FileLine at the given target index.
  memmove(&((*f_lines)[idx + 1]), &((*f_lines)[idx]),
          (*num_lines - idx) * sizeof(FileLine));

  (*f_lines)[idx].size = size;
  // malloc a buffer for the line in the new FileLine struct at the end.
  (*f_lines)[idx].line = malloc(size + 1);

  // copy the line into the malloc'ed buffer.
  memcpy((*f_lines)[idx].line, str, size);
  // null-terminate the line.
  (*f_lines)[idx].line[size] = '\0';

  // initialize the display line fields.
  (*f_lines)[idx].size_display = 0;
  (*f_lines)[idx].line_display = NULL;

  // initialize the display line for the new FileLine struct.
  File_SetLineDisplay(&((*f_lines)[idx]));

  (*num_lines)++;
}

FileLine *File_GetLines(const char *file_name, int *size) {
  // read in a single line from the given file.
  // returns a buffer up to the first \r or \n in the file.
  FILE *f_ptr = fopen(file_name, "r");
  if (f_ptr == NULL) {
    quit("fopen");
  }

  // malloc space for the array of FileLines
  // FileLine *lines = (FileLine *) malloc(sizeof(FileLine));
  // if (lines == NULL) {
  //   fclose(f_ptr);
  //   quit("malloc'ing FileLine");
  // }
  FileLine *lines = NULL;
  int num_lines = 0;

  // use getline to get the first line from the file.
  char *line = NULL;
  size_t line_capacity = 0;
  ssize_t line_size;

  while ((line_size = getline(&line, &line_capacity, f_ptr)) != -1) {
    while (line_size > 0 && (line[line_size - 1] == '\n' ||
                             line[line_size - 1] == '\r')) {
      // remove '\n' and '\r' characters from the end of the line.
      line_size--;
    }
    // inser the new FileLine at the end of the array.
    File_InsertFileLine(&lines, &num_lines, line, line_size, num_lines);
  }

  // set the output parameters with the number of lines read.
  *size = num_lines;

  // line will be reused by getline as a malloc'ed buffer, so only
  //  free it once.
  free(line);
  fclose(f_ptr);

  return lines;
  // int size;
  // char *file_str = File_ToString(file_name, &size);
  // if (file_str == NULL) {
  //   return NULL;
  // }
  // int idx = (int) (strchr(file_str, '\n') - file_str);
  // if (idx > 0)
  // file_str[idx] = '\0';
  // fprintf(stderr, "made it\n");
  // return file_str;
}

void File_FreeLines(FileLine *file_lines, int num_lines) {
  for (int i = 0; i < num_lines; i++) {
    // free the line from each FileLine struct.
    free(file_lines[i].line);
    free(file_lines[i].line_display);
    // fprintf(stderr, "LINE %d: ", i);
    // for (int j = 0; j < file_lines[i].size; j++) {      
    //   fprintf(stderr, "%c", file_lines[i].line[j]);
    // }
    // fprintf(stderr, "\n");
  }
  // free the malloc'ed FileLine array poitner.
  free(file_lines);
}

int File_RawToDispIdx(FileLine *f_line, int line_idx) {
  int res = 0;
  // only count characters to the left of line_idx
  for (int i = 0; i < line_idx; i++) {
    if (f_line->line[i] == TAB) {
      // res % TAB_SIZE: num cols we are to the right
      //  of last tab snap.
      // subtracting from (TAB_SIZE - 1) gives num cols currently
      //  we are to the left of the next tab snap. Adding to res
      //  counts just up to the left of next tab snap.
      res += (TAB_SIZE - 1) - (res % TAB_SIZE);
    }
    res++;
  }
  return res;
}

void File_InsertChar(FileLine *f_line, int idx, char new_char) {
  // validate the index.
  // if(idx < 0 || idx > f_line->size) {
  //   idx = f_line->size;
  // }
  idx = validate_idx(idx, f_line->size);

  // allocate space for the line plus 1 byte we're inserting plus
  //  the null-terminator.
  f_line->line = realloc(f_line->line, f_line->size + 2);
  // use memmove since the destination and source strings overlap.
  // shift chars in the line from idx (including idx) 1 spot to 
  //  the right to make room for the new char.
  memmove(&(f_line->line[idx + 1]), &(f_line->line[idx]),
          (f_line->size) - idx + 1);
  // increment the size of the line by 1 character.
  (f_line->size)++;
  // assign the new character to its position.
  f_line->line[idx] = new_char;

  // update the line_display field to account for the new character.
  File_SetLineDisplay(f_line);
}

void File_RemoveChar(FileLine *f_line, int idx) {
  idx = validate_idx(idx, f_line->size);
  // move over the characters to the right of idx by one space to the left
  //  (including '\0').
  memmove(&(f_line->line[idx]), &(f_line->line[idx + 1]),
          f_line->size - idx);
  f_line->size--;
  File_SetLineDisplay(f_line);
}

// free the malloc'ed buffers in the FileLine.
void File_FreeFileLineBufs(FileLine *f_line) {
  free(f_line->line);
  free(f_line->line_display);
}

void File_RemoveRow(FileLine *f_line, int *num_lines, int idx) {
  idx = validate_idx(idx, *num_lines);
  // free the line and display line buffers in the target
  //  FileLine being removed.
  File_FreeFileLineBufs(&(f_line[idx]));
  // copy over all the FileLines coming after the target index one
  //  position before them, overwriting the target FileLine.
  memmove(&(f_line[idx]), &(f_line[idx + 1]),
          (*num_lines - idx - 1) * sizeof(FileLine));
  // removed a line, so decrease the size of the FileLines array.
  (*num_lines)--;
}

// if idx is negative or greater than size, returns size; otherwise,
//  returns idx.
static int validate_idx(int idx, int size) {
  return (idx < 0 || idx > size) ? size : idx;
}

void File_AppendLine(FileLine *f_line, const char *str, size_t str_size) {
  // make room for the new string to append to f_line's line buffer.
  f_line->line = realloc(f_line->line, f_line->size + str_size + 1);
  // copy over the new string to f_line's line buffer.
  memcpy(&(f_line->line[f_line->size]), str, str_size);
  // update the size of the f_line's line buffer.
  f_line->size += str_size;
  f_line->line[f_line->size] = '\0';  // null-terminate the new line string.
  // update the line_display field from the new line string.
  File_SetLineDisplay(f_line);
}

// Split the FileLine at position row in the given FileLine array
//  at position col in that FileLine's line. Insert a new line
//  with the part of the line to the right of col below row.
//  num_lines is the number of FileLines in the f_line array.
//  After returning, num_lines is incremented, since a new
//  FileLine was added to the array.
void File_SplitLine(FileLine **f_line, int *num_lines, int row, int col) {
  if (col == 0) {
    // at the start of a line.
    // insert a brand new empty FileLine at position row 
    //  (above the current row).
    File_InsertFileLine(f_line, num_lines, "", strlen(""), row);
  } else {
    // alias for a FileLine to split in the array.
    FileLine *l_ptr = &((*f_line)[row]);
    // create a new line below the current cursor-highlighted line
    //  which contains the characters to the right of the cursor.
    File_InsertFileLine(f_line, num_lines, &(l_ptr->line[col]),
                        l_ptr->size - col, row + 1);
    // realloc in File_InsertFileLine might invalidate l_ptr,
    //  so reassign it here.
    l_ptr = &((*f_line)[row]);
    // remove characters on the current line by reducing the size.
    l_ptr->size = col;
    // null-terminate the new line string.
    l_ptr->line[l_ptr->size] = '\0';
    // update the diaply line according to the new line.
    File_SetLineDisplay(l_ptr);
  }
  // editor should increment row position and set col position to 0.
}
