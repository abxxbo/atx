/* includes */

#include <stdio.h>

#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* defines */
// Get ctrl key equiv of a character
#define CTRL_KEY(n) ((n) & 0x1f)

// other
#define CLR_SCRN()  write(STDOUT_FILENO, "\x1b[2J", 4);
#define RESET_CUR() write(STDOUT_FILENO, "\x1b[H", 3);

/* data */

struct _edit_conf {
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

/* output */
void _draw_rows() {
  for(int y = 0; y < E.screenrows; y++) write(STDOUT_FILENO, "~\r\n", 3);
}

void _refresh(){
  CLR_SCRN();
  RESET_CUR();
  _draw_rows();
  RESET_CUR();
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

int get_cur_pos(int *rows, int *cols){
  if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  printf("\r\n");
  char c;
  while(read(STDIN_FILENO, &c, 1) == 1){
    if(iscntrl(c)) printf("%d\r\n", c);
    else printf("%d ('%c')\r\n", c, c);
  }
  read_key();
  return -1;
}

int get_win_size(int* rows, int* colums){
  struct winsize ws;
  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return get_cur_pos(rows, colums);
  }
  *colums = ws.ws_col;
  *rows   = ws.ws_row;
  return 0;
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
  if(get_win_size(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
  // Enter raw mode
  _enable_raw();
  // Initialize editor
  _init_editor();

  while(1){
    CLR_SCRN();   // Clears buffer
    RESET_CUR();  // Repositions cursor
    process_key();
  }
  return 0;
}
