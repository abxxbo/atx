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

int check_for_file() {
	FILE* fp = fopen(".atxrc", "r");
	if(!fp) return 1; // File doesn't exist.
	fclose(fp);
	return 0;					// it do exist
}


// parse file
void parse_config() {
	int config = check_for_file();
	if(config == 1) exit(1);

	// Ok, file exists, not we can parse
	// just need to open it and then parse
	FILE* _fp = fopen(".atxrc", "r");

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
		// loop
		splitted = strtok(NULL, delim);
	}
}
