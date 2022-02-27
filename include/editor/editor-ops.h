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

#include "row-ops.h"

void insert_char_ed(int c){
  if(E.cy == E.numrows) {
    _appnd_row(E.numrows, "", 0);
  }
  insertchar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void insert_nline() {
  if(E.cx == 0){
    _appnd_row(E.cy, "", 0);
  } else {
    erow* row = &E.row[E.cy];
    _appnd_row(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    _update_row(row);
  }
  E.cy++;
  E.cx = 0;
}

void del_char_ed() {
  if(E.cy == E.numrows) return;
  if(E.cx == 0 && E.cy == 0) return;
  erow* row = &E.row[E.cy];
  if(E.cx > 0) {
    del_chr(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    appnd_to_end(&E.row[E.cy - 1], row->chars, row->size);
    del_row(E.cy);
    E.cy--;
  }
}
