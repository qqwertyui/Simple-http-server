#include "utils.h"
#include <stdio.h>
#include <string.h>

#define __internal_bufsz 11
static char __internal_buf[__internal_bufsz];

void hex_dump(unsigned char *data, size_t datasz) {
  for(size_t i=0; i<datasz; i++) {
    if((i % 8) == 0 && (i > 0) ) {
      puts("");
    }
    printf("%.2x ", data[i] );
  }
  puts("");
}

char* __uitoa(unsigned int number) {
  memset(__internal_buf, 0, __internal_bufsz); // at first call cleaning is not necessary but latter uses may need it
  sprintf(__internal_buf, "%u", number);

  return __internal_buf;
}
