#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void fatal_log(const char* fmt){
  fprintf(stderr, "[FATAL] %s\n", fmt);
  exit(2);
}
