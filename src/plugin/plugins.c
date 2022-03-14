#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_help() {
  fprintf(stderr, "atx-plugins [URL/dir]\n");
  fprintf(stderr, "  --help          print help and exit\n");
  exit(3);
}

int main(int argc, char** argv){
  // print help command
  if(strcmp(argv[1], "--help") == 0) print_help();

  // other

  // exit
  return 0;
}
