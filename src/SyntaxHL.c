#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>  // isspace

#include "SyntaxHL.h"

// null-terminate the array of extensions.
static char *EXTENSIONS_C[] = {".c", ".h", "\0"};

static char *KEYWORDS_C[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

Syntax langs[] = {
  {
    "c",
    EXTENSIONS_C,
    "//",
    KEYWORDS_C,
    HIGHLIGHT_NUMBERS | HIGHLIGHT_STRINGS
  }
};

// the length of the languages array.
#define NUM_LANGS (sizeof(langs) / sizeof(Syntax))

int File_GetHighlightCode(unsigned char h) {
  switch (h) {
    case HL_NUMBER:
      return 31;
    case HL_STRING:
      return 35;
    case HL_COMMENT:
      return 36;
    case HL_KEYWORD1:
      return 32;
    case HL_KEYWORD2:
      return 33;
    case HL_MATCH:
      return 34;
    default:
      return 0;
  }
}

static bool Is_Separator(char c) {
  // check if the given character is a space, null-termiantor, or any
  //  of the other separator symbols.
  return isspace(c) || c == '\0' || strchr(",.()+/*=~%<>[];", c) != NULL;
}

// puts the highlighting codes for the characters in the
//  given line (length l_size) into the given h_line 
//  according to the given syntax.
void Syntax_SetHighlight(Syntax *syntax, char *line,
                         int l_size, unsigned char **h_line) {
  // resize the highlight buffer, since line_display may have changed.
  // the highlight array has the same size as line_display.
  *h_line = realloc(*h_line, l_size);
  // set all characters in highlight to default.
  memset(*h_line, HL_NORMAL, l_size);

  if (syntax == NULL) {
    // no syntax specified for the file, so return after setting
    //  all highlighting to default in the memset above.
    return;
  }

  // alias for the single line comment delimiter.
  char *cd_single = syntax->comment_delim_single;
  int cd_single_size = (cd_single != NULL) ? strlen(cd_single) : 0;

  // alias for keywords array.
  char **keywords = syntax->keywords;

  // track whether the previous character was a separator, in order
  //  to tell if the current character is part of a new sequence.
  // initialize to true since the first word is part of a new word.
  bool prev_sep = true;
  // true if the current char is part of a string (starts and ends
  //  with double quotes "")
  bool in_string = false;
  // the current string delimiter (either ' or ", or '\0' if not in a string).
  char str_delim = '\0';
  
  // set the color codes for the line_display characters.
  int i = 0;
  while (i < l_size) {
    // alias for the current line character.
    char c = line[i];
    // the type of highlight of the previous character.
    unsigned char prev_h_char = (i > 0) ? (*h_line)[i - 1] : HL_NORMAL;

    if (cd_single_size != 0 && !in_string) {
      // syntax specifies comment highlighting, and we're not in a string.
      if (!strncmp(&(line[i]), cd_single, cd_single_size)) {
        // encountered the start of a single line comment, so set the
        //  rest of the line for comment highlighting and break.
        memset(&((*h_line)[i]), HL_COMMENT, l_size - i);
        break;
      }
    }

    if (syntax->flags & HIGHLIGHT_NUMBERS) {
      // the syntax specifies number highlighting.
      if ((isdigit(c) && (prev_sep || prev_h_char == HL_NUMBER)) ||
          (c == '.' && prev_h_char == HL_NUMBER)) {
        // in order to be highlighted, a character must be a digit
        //  and the previous char must be either a separator or
        //  also highlighted, or a character must be a dot '.'
        //  and the previous char must be highlighted.
        (*h_line)[i] = HL_NUMBER;
        // consume this character.
        i++;
        // the just consumed char was not a separator.
        prev_sep = false;
        continue;
      }
    }

    if (syntax->flags & HIGHLIGHT_STRINGS) {
      if (in_string) {
        (*h_line)[i] = HL_STRING;
        if (c == '\\' && i + 1 < l_size) {
          // encountered an escaped character.
          i += 2;  // consume '\' and the escaped char.
          continue;
        }
        if (c == str_delim) {
          // encountered the matching delimiter, so we are not in a string.
          str_delim = '\0';
          in_string = false;
        }
        i++;
        // closing quote is a separator.
        prev_sep = true;
        continue;
      } {
        if (c == '"' || c == '\'') {
          // encountered the start of a string, so save the opening delimiter.
          str_delim = c;
          in_string = true;
          (*h_line)[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }

    if (prev_sep) {
      // keywords need a separator before them.
      int j;  // index into the keywords array.
      for (j = 0; keywords[j] != NULL; j++) {
        int keyword_len = strlen(keywords[j]);
        bool is_type_2 = keywords[j][keyword_len - 1] == '|';
        if (is_type_2) {
          // remove the last char if it is a type 2 keyword.
          keyword_len--;
        }

        if (!strncmp(&(line[i]), keywords[j], keyword_len) &&
            Is_Separator(line[i + keyword_len])) {
          // check if the current keyword exists at this point int the line,
          //  and that it is followed by a separator (\0 EOL counts as a separator).
          // unsigned char keyword_type = (is_type_2) ? HL_KEYWORD2 : HL_KEYWORD1;
          memset(&((*h_line)[i]), (is_type_2) ? HL_KEYWORD2 : HL_KEYWORD1, keyword_len);
          i += keyword_len;
          break;
        }
      }

      if (keywords[j] != NULL) {
        // encountered a keyword in the above loop.
        prev_sep = false;
        continue;
      }
    }

    prev_sep = Is_Separator(c);
    i++;
  }
}

void Syntax_LangFromFile(const char *file_name, Syntax **syntax) {
  if (file_name == NULL) {
    return;
  }

  // reverse search through the filename for the dot . which marks the
  //  start of the extension.
  char *ext = strrchr(file_name, '.');

  // iterate over the array of known languages.
  for (uint32_t i = 0; i < NUM_LANGS; i++) {
    // set the output parameter to hold the current language being
    //  considered.
    *syntax = &(langs[i]);
    // iterate over each associated extension pattern for the language.
    // the array of extensions is null-terminated.
    for (uint32_t j = 0; (*syntax)->extensions[j] != NULL; j++) {
      // determine if the pattern is a file extension pattern.
      bool is_ext = ((*syntax)->extensions[j][0] == '.');

      // if the pattern is an extension pattern, it's a match only if it has
      //  the same extension as the file_name.
      // if the pattern is not an extension pattern (eg Makefile has no extension),
      //  match only if the pattern appears anywhere in the file_name.
      if ((is_ext && ext != NULL && strcmp(ext, (*syntax)->extensions[j]) == 0) ||
          (!is_ext && strstr(file_name, (*syntax)->extensions[j]))) {
        // a match was found. the output parameter was already set, so return.
        return;
      }
    }
  }
  // no known languages match the file extension of file_name.
  *syntax = NULL;
}
