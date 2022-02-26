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

#include "input.h"
#include "editor.h"
#include "highlighting.h"
#include "terminal.h"

/* append buffer */
#define ABUF_INIT {NULL, 0}
struct abuf {
  char* b;
  int len;
};

void ab_append(struct abuf* ab, const char* fmt, int len){
  char* new = realloc(ab->b, ab->len + len);
  if(new == NULL) return;
  memcpy(&new[ab->len], fmt, len);
  ab->b = new;
  ab->len += len;
}

void ab_free(struct abuf *ab){
  free(ab->b);
}

/* output */
void edit_scroll(){
  E.rx = E.cx;
  // Vert
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }

  // Horiz
  if(E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if(E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void _draw_rows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "ATX editor v%s", ATX_VER);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          ab_append(ab, "~", 1);
          padding--;
        }
        while (padding--) ab_append(ab, " ", 1);
        ab_append(ab, welcome, welcomelen);
      } else {
        ab_append(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if(len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      char* c = &E.row[filerow].render[E.coloff];
	  	unsigned char* hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      for(int j = 0; j<len; j++){
        if(hl[j] == HL_NORMAL){
          if(current_color != -1){
            ab_append(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          ab_append(ab, &c[j], 1);
        } else {
          int color = syntax_to_cl(hl[j]);
          if(color != current_color){
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            ab_append(ab, buf, clen);
          }
          ab_append(ab, &c[j], 1);
				}
			}
      ab_append(ab, "\x1b[39m", 5);
    }

    ab_append(ab, "\x1b[K", 3);
    ab_append(ab, "\r\n", 2);
  }
}

// status bar!
void status_bar(struct abuf* ab){
  ab_append(ab, "\x1b[7m", 4); // invert colors
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "File: %s %s",
        E.filename ? E.filename : "Untitled", E.dirty ? "(modified)" : "");
  
  
  int rlen;
  if(E.numrows != 404){
    rlen = snprintf(rstatus, sizeof(rstatus), "Line %d of %d (ft: %s)",
      E.cy + 1, E.numrows, E.e_syntax ? E.e_syntax->filetype : "no ft");
  } else {
    rlen = snprintf(rstatus, sizeof(rstatus), "Line %d of [LINES NOT FOUND] (ft: %s)", E.cy + 1, E.e_syntax ? E.e_syntax->filetype : "no ft");
  }
  if(len > E.screencols) len = E.screencols;
  ab_append(ab, status, len);
  while(len < E.screencols) {
    if(E.screencols - len == rlen) {
      ab_append(ab, rstatus, rlen);
      break;
    } else {
      ab_append(ab, " ", 1);
      len++;
    }
  }
  ab_append(ab, "\x1b[m", 3); // revert to normal
}

void _refresh() {
  edit_scroll();
  struct abuf ab = ABUF_INIT;

  ab_append(&ab, "\x1b[?25l", 6);
  ab_append(&ab, "\x1b[H", 3);

  _draw_rows(&ab);
  status_bar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  ab_append(&ab, buf, strlen(buf));

  ab_append(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}

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
  if(strcmp(argv[1], "--help") == 0){
    printf("Usage: atx [file]\n\n");
    printf("Available arguments:\n--help => this message\n\n");
    printf("Keybindings:\nCtrl+S -- save\nCtrl+Q -- quit\n");
    exit(0);
  }
  // Enter raw mode
  _enable_raw();
  // Initialize editor
  _init_editor();

  if(argc >= 2){
    open_file(argv[1]);
  }

  while(1){
    _refresh();     // Clears buffer
    process_key();  // Repositions cursor
  }
  return 0;
}
