#include "Quit.h"
#include "IOUtils.h"

int ReadFD(int fd, unsigned char *buf, int num_chars) {
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