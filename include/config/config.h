#pragma once

#include <stdio.h>
#include <stdlib.h>

/* Config file: .atxrc
 * ft:           atx conf
 * supported changes:
 *    => tab size
 * ---------------------
 *  You're able to change this
 *  via
 *  [ KEY ] => [ VALUE ]
 *  so, for example:
 *  tabs => 2 (default)
*/


// get full file contents
char* ffile_contents(){
  FILE* fp = fopen(".atxrc", "r");
  char* buffer = 0;
  long length;
  if(fp) {
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if(buffer) fread(buffer, 1, length, f);
    fclose(fp);
  }
  return buffer;
}
