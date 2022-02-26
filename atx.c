#include <stdio.h>

#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// Raw mode
struct termios orig_termios;

void disable_raw() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void _enable_raw() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw);

  struct termios raw = orig_termios;
  
  // flags
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  
  // Timeout for read
  raw.c_cc[VMIN]  = 0;
  raw.c_cc[VTIME] = 1;
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  // Enter raw mode
  _enable_raw();

  while(1){
    char c = '\0';
    read(STDIN_FILENO, &c, 1);
    if(iscntrl(c)){
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
    // Exit on q
    if(c == 'q') break;
  }
  return 0;
}
