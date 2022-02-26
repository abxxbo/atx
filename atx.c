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
  raw.c_iflag &= ~(IXON);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  // Enter raw mode
  _enable_raw();

  char c;
  // Use 'q' to quit
  while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
    if(iscntrl(c)){
      printf("%d\n", c);
    } else {
      printf("%d ('%c')\n", c, c);
    }
  }
  return 0;
}
