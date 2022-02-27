// im too lazy to change this file name
// so shut up
#pragma once

#include <stdio.h>

#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "editor.h"
#include "input.h"

struct _edit_conf E;

char* editor_prompt(char* prompt);

void editor_find() {
  char *query = editor_prompt("Search: %s (ESC to cancel)");
  if (query == NULL) return;

  int i;
  for (i = 0; i < E.numrows; i++) {
    erow *row = &E.row[i];
    char *match = strstr(row->render, query);
    if (match) {
      E.cy = i;
      E.cx = match - row->render;
      E.rowoff = E.numrows;
      break;
    }
  }

  free(query);
}

void editor_exec() {
  char* command = editor_prompt("Type command: %s (ESC to cancel)");
  // tee hee
  // TODO: fix later
  system(command);
}
