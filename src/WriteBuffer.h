#include <stdlib.h>
#include <string.h>

typedef struct buf {
  // pointer to buffer in memory.
  unsigned char *buffer;
  // size of the buffer.
  int size;
} Buffer;

#define EMPTY_BUF {NULL, 0}

// Appends the given sequence of characters (str) of size 'size'
//  to the end of the given Buffer struct. Updates its size and 
//  realloc's the buffer if needed.
void WriteBuffer_Append(Buffer *buf, unsigned char *str, int size);

// Frees a Buffer struct's heap allocated buffer.
void WriteBuffer_Free(Buffer *buf);
