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

static int File_Open(const char *file_name, int *fd, int *size);

char *File_ToString(const char *file_name, int *size) {
  int fd, f_size;
  if (File_Open(file_name, &fd, &f_size) == -1) {
    return NULL;
  }

  // malloc space for the null terminator.
  char *buf = (char *) malloc(f_size + 1);
  if (buf == NULL) {
    // malloc failed; close the file
    close(fd);
    return NULL;
  }

  int res = WrappedRead(fd, buf, f_size);
  if (res == -1) {
    // fatal read error.
    close(fd);
    free(buf);
    return NULL;
  }

  close(fd);
  *size = f_size - res;

  // null terminate the string.
  buf[*size] = '\0';
  return buf;
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

// Appned the given string 'str' with the given size 'size'
//  to the given array of FileLines as a new FileLine struct.
//  'num_lines' is the number of FileLine objects in 'f_lines'.
//  'num_lines' is incremented by 1 after the operation.
static void File_AppendLine(FileLine **f_lines, int *num_lines,
                            const char *str, size_t size) {
  *f_lines = realloc(*f_lines, (*num_lines + 1) * sizeof(FileLine));

  int at = *num_lines;
  (*f_lines)[at].size = size;
  // malloc a buffer for the line in the new FileLine struct at the end.
  (*f_lines)[at].line = malloc(size + 1);

  // copy the line into the malloc'ed buffer.
  memcpy((*f_lines)[at].line, str, size);
  // null-terminate the line.
  (*f_lines)[at].line[size] = '\0';

  // initialize the display line fields.
  (*f_lines)[at].size_display = 0;
  (*f_lines)[at].line_display = NULL;

  File_SetLineDisplay(&((*f_lines)[at]));

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
    File_AppendLine(&lines, &num_lines, line, line_size);
  }

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

static int File_Open(const char *file_name, int *fd, int *size) {
  struct stat f_stat;
  // make sure the given file name is a valid regular file.
  int res = stat(file_name, &f_stat);
  if (res != 0 || !S_ISREG(f_stat.st_mode)) {
    // the file doesn't exist or is not a regular file.
    return -1;
  }

  *fd = open(file_name, O_RDONLY);
  if (*fd == -1) {
    // open failed.
    return -1;
  }
  *size = f_stat.st_mode;
  return 0;
}
