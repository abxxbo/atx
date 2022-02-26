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

void die(const char* fmt){
  CLR_SCRN();
  RESET_CUR();
  perror(fmt);
  exit(1);
}

void disable_raw() {
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void _enable_raw() {
  if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disable_raw);

  struct termios raw = E.orig_termios;

  // flags
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // Timeout for read
  raw.c_cc[VMIN]  = 0;
  raw.c_cc[VTIME] = 1;

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int get_cur_pos(int *rows, int *cols){
  char buf[32];
  unsigned int i = 0;

  if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while(i < sizeof(buf) - 1){
    if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if(buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if(buf[0] != '\x1b' || buf[1] != '[') return -1;
  if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

int get_win_size(int* rows, int* colums){
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return get_cur_pos(rows, colums);
  }
  *colums = ws.ws_col;
  *rows   = ws.ws_row;
  return 0;
}
