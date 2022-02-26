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
#include "highlighting.h"

struct _edit_conf E;

void _update_row(erow * row){
  int tabs = 0;
  for(int j = 0; j < row->size; j++) if(row->chars[j] == '\t') tabs++;
  free(row->render);
  row->render = malloc(row->size + tabs*(ATX_TAB - 1) + 1);

  int idx = 0;
  for(int j = 0; j < row->size; j++){
    if(row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while(idx % ATX_TAB != 0) row->render[idx++] = ' ';
    } else row->render[idx++] = row->chars[j];
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  update_syntax(row);
}

void _appnd_row(int at, char* s, size_t len){
  if(at < 0 || at > E.numrows) return;

  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for(int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

  E.row[at].idx = at;

  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize  = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  E.row[at].hl_open_comment = 0;
  _update_row(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void free_row(erow *row){
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void del_row(int at){
  if(at < 0 || at >= E.numrows) return;
  free_row(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  for(int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void insertchar(erow* row, int at, int c){
  if(at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  _update_row(row);
  E.dirty++;
}

void appnd_to_end(erow* row, char* s, size_t len){
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  _update_row(row);
  E.dirty++;
}

void del_chr(erow *row, int at){
  if(at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  _update_row(row);
  E.dirty++;
}
