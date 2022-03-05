/* includes */

// Since our compiler complains about
// getline() being implicit, we need
// to define some feature test macros
// Even if it doesn't complain, these
// are good for compability
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>

// editor/
#include "input.h"
#include "editor.h"
#include "highlighting.h"
#include "terminal.h"

// config
#include "config/config.h"

/* init */
void _init_editor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.e_syntax = NULL;
  if(get_win_size(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  E.screenrows -= 1;
}

int main(int argc, char** argv) {
  // set tabs to 2, i'm an idiot
  if(argc >= 2){ 
    if(strcmp(argv[1], "--help") == 0){
      printf("Usage: atx [file]\n\n");
     	printf("Available arguments:\n--help => this message\n\n");
    	printf("Keybindings:\nCtrl+S -- save\nCtrl+Q -- quit\n\nCtrl+F -- find\n");
 	    printf("Ctrl+E -- exec command\n\n\nControl key help:\n");
  		printf("Ctrl+E -- output of command is stored in /tmp/file_out.txt\n");
	  	printf("Ctrl+F -- it will go to the first instance of thing to search\n");
		  printf("Ctrl+S -- common sense..\nCtrl+Q -- common sense..\n");
	    exit(0);
 	  }
  }

	// Before we do anything, parse config
	parse_config(); 
	// Enter raw mode
  _enable_raw();
  // Initialize editor
  _init_editor();

	if(argc == 1){
		open_file("/tmp/Untitled");
	} else if(argc >= 2){
  	open_file(argv[1]);
  }
  while(1){
    _refresh(eob);  // Clears buffer
    process_key();  // Repositions cursor
  }
  return 0;
}
