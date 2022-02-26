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

#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/* defines */

// atx version
#define ATX_VER "0.0.1-beta"
#define ATX_TAB 2           // 8 < 2 in terms of tabs

// Get ctrl key equiv of a character
#define CTRL_KEY(n) ((n) & 0x1f)

// other
#define CLR_SCRN()  write(STDOUT_FILENO, "\x1b[2J", 4);
#define RESET_CUR() write(STDOUT_FILENO, "\x1b[H", 3);

// block some keys
enum ed_keys {
  BACKSPACE = 127,
  ARR_L = 1000,
  ARR_R,
  ARR_U,
  ARR_D,
  DEL_KEY
};

/* data */

typedef struct erow {
  int size;
  int rsize;
  char* chars;
  char* render;
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
  struct termios orig_termios;
};
struct _edit_conf E;

/* terminal */

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
}

void _appnd_row(char* s, size_t len){
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));

  int at = E.numrows;
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.numrows++;

  E.row[at].rsize  = 0;
  E.row[at].render = NULL;
  _update_row(&E.row[at]);
}

void insertchar(erow* row, int at, int c){
  if(at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  _update_row(row);
}

/* editor operations */
void insert_char_ed(int c){
  if(E.cy == E.numrows) {
    _appnd_row("", 0);
  }  
  insertchar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

/* file io */
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
  FILE* fp = fopen(file, "r");
  if(!fp) die("fopen");

  char* line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while((linelen = getline(&line, &linecap, fp)) != -1){
    while(linelen > 0 && (line[linelen - 1] == '\n' ||
                          line[linelen - 1] == '\r'))
      linelen--;
    _appnd_row(line, linelen);
  }
  free(line);
  fclose(fp);
}

void save_file() {
  if(E.filename == NULL) return;

  int len;
  char* buf = rows_to_str(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  ftruncate(fd, len);
  write(fd, buf, len);
  close(fd);
  free(buf);
}

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
      ab_append(ab, &E.row[filerow].render[E.coloff], len);
    }

    ab_append(ab, "\x1b[K", 3);
    ab_append(ab, "\r\n", 2);
  }
}

// status bar!
void status_bar(struct abuf* ab){
  ab_append(ab, "\x1b[7m", 4); // invert colors
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "File: %s",
        E.filename ? E.filename : "Untitled");
  
  
  int rlen;
  if(E.numrows != 404){
    rlen = snprintf(rstatus, sizeof(rstatus), "Line %d of %d",
      E.cy + 1, E.numrows);
  } else {
    rlen = snprintf(rstatus, sizeof(rstatus), "Line %d of [LINES NOT FOUND]",
        E.cy + 1);
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

/* input */
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
      // ...
      break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      // ...
      break;

    // exit
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
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

/* init */
void _init_editor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.filename = NULL;
  if(get_win_size(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  E.screenrows -= 1;
}

int main(int argc, char* argv[]) {
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
