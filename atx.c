/* includes */

#include <stdio.h>

#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

/* defines */

// atx version
#define ATX_VER "0.0.1-beta"

// Get ctrl key equiv of a character
#define CTRL_KEY(n) ((n) & 0x1f)

// other
#define CLR_SCRN()  write(STDOUT_FILENO, "\x1b[2J", 4);
#define RESET_CUR() write(STDOUT_FILENO, "\x1b[H", 3);

/* data */

struct _edit_conf {
  // Cursor
  int cx, cy;
  // Screen
  int screenrows;
  int screencols;
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
void _draw_rows(struct abuf* ab) {
  for(int y = 0; y < E.screenrows; y++){
    if(y == E.screenrows / 3){
      // welcome!
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
                                "ATX v%s", ATX_VER);
      if(welcomelen > E.screencols) welcomelen = E.screencols;
      int padding = (E.screencols - welcomelen) / 2;
      if(padding) {
        ab_append(ab, "~", 1);
        padding--;
      }
      while(padding --) ab_append(ab, " ", 1);
      ab_append(ab, welcome, welcomelen);
    } else ab_append(ab, "~", 1);

    ab_append(ab, "\x1b[K", 3);
    if(y < E.screenrows - 1) ab_append(ab, "\r\n", 2);
  }
}

void _refresh(){
  struct abuf ab = ABUF_INIT;

  ab_append(&ab, "\x1b[?25l", 6);
  ab_append(&ab, "\x1b[H", 3);

  _draw_rows(&ab);

  ab_append(&ab, "\x1b[H", 3);
  ab_append(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}

/* input */
char read_key() {
  int nread;
  char ch;
  while((nread = read(STDIN_FILENO, &ch, 1)) != 1) {
    if(nread == -1 && errno != EAGAIN) die("read");
  }
  return ch;
}

void process_key() {
  char ch = read_key();
  switch(ch){
    case CTRL_KEY('q'): // Ctrl-q
      CLR_SCRN();
      RESET_CUR();
      exit(0);
      break;
  }
}

/* init */
void _init_editor() {
  E.cx = 0;
  E.cy = 0;
  if(get_win_size(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
  // Enter raw mode
  _enable_raw();
  // Initialize editor
  _init_editor();

  while(1){
    _refresh();     // Clears buffer
    process_key();  // Repositions cursor
  }
  return 0;
}
