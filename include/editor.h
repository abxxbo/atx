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

#define ATX_VER "0.0.1-beta"
#define ATX_TAB 2

#define CTRL_KEY(n) ((n) & 0x1f)

#define CLR_SCRN()  write(STDOUT_FILENO, "\x1b[2J", 4);
#define RESET_CUR() write(STDOUT_FILENO, "\x1b[H", 3);

struct edit_syntax {
  char* filetype;
  char** filematch;
  char** keywords;
  char* singline_comment_start;
  char* multiline_comment_start;
  char* multiline_comment_end;
  int flags;
};

typedef struct erow {
  int idx;
  int hl_open_comment;
  int size;
  int rsize;
  char* chars;
  char* render;
  unsigned char* hl;
} erow;

struct _edit_conf {
  // Cursor
  int cx, cy;
  // Tabs
  int rx;     // Tabs + cursor impl
  // Scrolling
  int rowoff; // vert
  int coloff; // horiz
  // Screen
  int screenrows;
  int screencols;
  int numrows;
  erow* row;
  // status bar
  char* filename;
  // is modified?
  int dirty;    // 0 = clean ; 1 = dirty
  // editor syntax
  struct edit_syntax* e_syntax;
  struct termios orig_termios;
};

