#ifndef WRITE_BUFFER_H_
#define WRITE_BUFFER_H_

typedef struct buf {
  // pointer to buffer in memory.
  unsigned char *buffer;
  // size of the buffer.
  int size;
} Buffer;

#define EMPTY_BUF {NULL, 0}

// void WB_AppendCmd(Buffer *buf, unsigned char *str);

// Appends the given sequence of characters (str) of size 'size'
//  to the end of the given Buffer struct. Updates its size and 
//  realloc's the buffer if needed. Caller must free the buffer
//  allocated at some time after the first call to WB_Append.
void WB_Append(Buffer *buf, unsigned char *str, int size);

// Append the given escape command.
void WB_AppendESCCmd(Buffer *wbuf, unsigned char *cmd);

// Frees a Buffer struct's heap allocated buffer.
void WB_Free(Buffer *buf);

// Writes the entire contents of buf to stdout.
void WB_Write(Buffer *buf);

#endif  // WRITE_BUFFER_H_
