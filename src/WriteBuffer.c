#include "WriteBuffer.h"

void WriteBuffer_Append(Buffer *wbuf, unsigned char *str, int size) {
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

void WriteBuffer_Free(Buffer *wbuf) {
  free(wbuf->buffer);
}
