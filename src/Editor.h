#ifndef EDITOR_H_
#define EDITOR_H_

// Initializes the terminal for editing.
void Editor_Open(void);

// Returns the terminal to its original state.
void Editor_Close(void);

// Invokes the appropriate operation for a key read from input.
void Editor_InterpretKeypress(void);

void Editor_Refresh(void);

#endif  // EDITOR_H_
