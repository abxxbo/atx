#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void non_fatal_log(const char* fmt){
  fprintf(stderr, "[NON-FATAL] %s\n", fmt);
}
