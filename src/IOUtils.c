#include <errno.h>
#include <unistd.h>  // for POSIX read

#include "Quit.h"

#include "IOUtils.h"
#include "ESCCommands.h"

int WrappedRead(int fd, unsigned char *buf, int num_chars) {
  int ret_val;
  while (1) {
    ret_val = read(fd, buf, num_chars);
    if (ret_val == -1 && (errno == EAGAIN || errno == EINTR)) {
      continue;
    }
    break;
  }
  return ret_val;
}

// rewrite.
int WrappedWrite(int fd, const unsigned char *buf, int num_bytes) {
  int write_res, bytes_written = 0;

  while (bytes_written < num_bytes) {
    write_res = write(fd, buf + bytes_written, num_bytes - bytes_written);
    if (write_res == -1) {
      if ((errno == EAGAIN) || (errno == EINTR)) continue;
      break;
    }
    if (write_res == 0) break;
    bytes_written += write_res;
  }
  return bytes_written;
}

