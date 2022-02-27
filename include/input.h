#pragma once

enum ed_keys {
  BACKSPACE = 127,
  ARR_L = 1000,
  ARR_R,
  ARR_U,
  ARR_D,
  DEL_KEY
};

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

#include "file-io.h"
#include "terminal.h"
#include "find.h"

int read_key() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
        case 'A': return ARR_U;
        case 'B': return ARR_D;
        case 'C': return ARR_R;
        case 'D': return ARR_L;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}


void editor_mv_cur(int key) {
  erow* row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case ARR_L:
      if(E.cx != 0) E.cx--;
      else if(E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARR_R:
      if(row && E.cx < row->size) E.cx++;
      else if(row && E.cx == row->size){
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARR_U:
      if(E.cy != 0) E.cy--;
      break;
    case ARR_D:
      if(E.cy < E.numrows) E.cy++;
      break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) E.cx = rowlen;
}

void process_key(){
  int c = read_key();

  switch (c) {
    case '\r':
      insert_nline();
      break;
    case BACKSPACE:
    case DEL_KEY:
      if(c == DEL_KEY) editor_mv_cur(ARR_R);
      del_char_ed();
      break;

    // exit
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    
    // keyword finder
    case CTRL_KEY('f'):
      editor_find();
      break;
    // save file
    case CTRL_KEY('s'):
      save_file();
      break;

    // other
    case ARR_U:
    case ARR_D:
    case ARR_L:
    case ARR_R:
      editor_mv_cur(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;
    default:
      insert_char_ed(c);
      break;
  }
}

void set_status(const char *fmt, ...);
char* editor_prompt(char* prompt);

void set_status(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
}

char *editor_prompt(char *prompt) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while (1) {
    set_status(prompt, buf);
    _refresh();

    int c = read_key();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      set_status("");
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        set_status("");
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
  }
}
