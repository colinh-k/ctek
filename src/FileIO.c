#include "FileIO.h"
#include "IOUtils.h"
#include "Quit.h"

#include <sys/stat.h>  // for struct stat
#include <fcntl.h>
#include <stdlib.h>  // for NULL
#include <string.h>  // for strchr
#include <unistd.h>  // for close
#include <stdio.h> 

static int File_Open(const char *file_name, int *fd, int *size);

char *File_ToString(const char *file_name, int *size) {
  // struct stat f_stat;
  // // make sure the given file name is a valid regular file.
  // int res = stat(file_name, &f_stat);
  // if (res != 0 || !S_ISREG(f_stat.st_mode)) {
  //   // the file doesn't exist or is not a regular file.
  //   return NULL;
  // }

  // int fd = open(file_name, O_RDONLY);
  // if (fd == -1) {
  //   // failed to open the file.
  //   return NULL;
  // }

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

// Appned the given string 'str' with the given size 'size'
//  to the given array of FileLines as a new FileLine struct.
//  'num_lines' is the number of FileLine objects in 'f_lines'.
//  'num_lines' is incremented by 1 after the operation.
static void File_AppendLine(FileLine **f_lines, int *num_lines,
                            const char *str, size_t size) {
  *f_lines = realloc(*f_lines, (*num_lines + 1) * sizeof(FileLine));

  int at = *num_lines;
  (*f_lines)[at].size = size;
  (*f_lines)[at].line = malloc(size + 1);

  memcpy((*f_lines)[at].line, str, size);
  (*f_lines)[at].line[size] = '\0';
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
  ssize_t line_size = getline(&line, &line_capacity, f_ptr);

  if (line_size != -1) {
    while (line_size > 0 && (line[line_size - 1] == '\n' ||
                             line[line_size - 1] == '\r')) {
      // remove '\n' and '\r' characters from the end of the line.
      line_size--;
    }
    File_AppendLine(&lines, &num_lines, line, line_size);
  }

  *size = num_lines;

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
