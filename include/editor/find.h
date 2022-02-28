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
 	char path[1035]; 
	// note that this is fixed, due to Issue #1.
	FILE* fp;		// store in /tmp/file_out.txt
	FILE* tmp;	// temporary, used only for getting output.
	fp = popen(command, "r");
	if(!fp) {
		// set editor prompt to  'failed to run command'
		editor_prompt("Failed to run command");
	}
	// create the file
	system("touch /tmp/file_out.txt");
	tmp = fopen("/tmp/file_out.txt", "w");
	while(fgets(path, sizeof(path), fp) != NULL){
		// write to file
		fputs(path, tmp);
	}
	pclose(fp);
	fclose(tmp);
}
