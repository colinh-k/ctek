#ifndef SYNTAX_HL_H_
#define SYNTAX_HL_H_

// provides functionality for syntax highlighting 
//  an array of characters.

#include <stdint.h>  // for standard int types

// bit flags to define what char sequences should be highlighted.
#define HIGHLIGHT_NUMBERS (1<<0)

typedef struct {
  // the name of the language cooresponding to this syntax.
  char *language;
  // an array of extensions which identify a file of this syntax type.
  char **extensions;
  // a collection of flags for this syntax describing what
  //  char sequences should be highlighted.
  int32_t flags;
} Syntax;

// the codes representing color types for syntax highlighting.
//  these define the values a FileLine's highligh array
//  can contain.
typedef enum {
  HL_NORMAL = 0,
  HL_NUMBER,
  HL_MATCH
} Highlight_t;

// Identifies the syntax of the given file_name based
//  on its extension. Only identifies a language if it exists
//  in the list of specified languages. Sets the fields
//  of the given Syntax struct upon successful identification,
//  or sets it to NULL otherwise.
void Syntax_LangFromFile(const char *file_name, Syntax **syntax);

// puts the highlighting codes for the characters in the
//  given line (length l_size) into the given h_line 
//  according to the given syntax.
void Syntax_SetHighlight(Syntax *syntax, char *line,
                         int l_size, unsigned char **h_line);

int File_GetHighlightCode(unsigned char h);

#endif  // SYNTAX_HL_H_
