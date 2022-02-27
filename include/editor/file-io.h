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
#include "editor-ops.h"
#include "highlighting.h"

struct _edit_conf E;

char* rows_to_str(int* buflen){
  int totlen = 0;
  for(int j = 0; j < E.numrows; j++) totlen += E.row[j].size + 1;
  *buflen = totlen;

  char* buf = malloc(totlen);
  char* p = buf;
  for(int j = 0; j < E.numrows; j++){
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void open_file(char* file) {
  free(E.filename);
  E.filename = strdup(file);

  sel_syn();

  FILE* fp = fopen(file, "r");
  if(!fp){
    // Create the file
    FILE* fp2 = fopen(file, "a");
    fclose(fp2);
    fp = fopen(file, "r");
  }

  char* line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while((linelen = getline(&line, &linecap, fp)) != -1){
    while(linelen > 0 && (line[linelen - 1] == '\n' ||
                          line[linelen - 1] == '\r'))
      linelen--;
    _appnd_row(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}

void save_file() {
  if(E.filename == NULL) return;
  sel_syn();

  int len;
  char* buf = rows_to_str(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  ftruncate(fd, len);
  write(fd, buf, len);
  close(fd);
  E.dirty = 0;
  free(buf);
}
