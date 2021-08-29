#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "IOUtils.h"

#include "WriteBuffer.h"

void WB_AppendESCCmd(Buffer *wbuf, unsigned char *cmd) {
  WB_Append(wbuf, cmd, strlen((const char *) cmd));
}

void WB_Append(Buffer *wbuf, unsigned char *str, int size) {
  // allocate an expanced buffer for the new characters.
  unsigned char *expanded = (unsigned char *)
                            realloc(wbuf->buffer, wbuf->size + size);

  if (expanded == NULL) {
    // realloc failed.
    return;
  }

  // copy the new characters to the end of the new buffer location.
  memcpy(&expanded[wbuf->size], str, size);
  // update the Buffer metadata.
  wbuf->buffer = expanded;
  wbuf->size += size;
}

void WB_Free(Buffer *wbuf) {
  free(wbuf->buffer);
}

void WB_Write(Buffer *buf) {
  WrappedWrite(STDOUT_FILENO, buf->buffer, buf->size);
}
