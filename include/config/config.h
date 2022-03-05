#pragma once

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "other/logger.h"

#define ATXRC_CONFIG_LOCATION "/home/$(whoami)/.atxrc"

void parse_config() {
  system((char*)malloc(1 + strlen("touch ") + strlen(ATXRC_CONFIG_LOCATION)));

  // Ok, file exists, not we can parse
	// just need to open it and then parse
	FILE* _fp = fopen(ATXRC_CONFIG_LOCATION, "r");
  if(_fp) return;

	// Put into a variable
	char* buffer = 0;
	long length;
	if(_fp){
		fseek(_fp, 0, SEEK_END);
		length = ftell(_fp);
		fseek(_fp, 0, SEEK_SET);
		buffer = malloc(length);
		if(buffer) fread(buffer, 1, length, _fp);
		fclose(_fp);
	}

	// Parsing time
	char* delim = " ";	// might be bad practice
	char* splitted = strtok(buffer, delim);
	int is_tabs = 0;
	while(splitted != NULL){
		if(strcmp(splitted, "tabs") == 0){
			is_tabs = 1;
		}
		if(atoi(splitted) != 0 && is_tabs == 1){
			set_tabs_to(atoi(splitted));
		}

    // EOB character
    if(strcmp(splitted, "eob_char") == 0){
      if(atoi(splitted) == 0 && is_tabs == 0) {
        set_eob(splitted);
      }
    }
		// loop
		splitted = strtok(NULL, delim);
	}
}

// if config file has nothing in it
// delete it.
void clean_up_config() {
  FILE* fp = fopen(ATXRC_CONFIG_LOCATION, "r");
  long size;
  if(fp != NULL){
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    if(size == 0){
      system("rm .atxrc");
    }
  }
}
