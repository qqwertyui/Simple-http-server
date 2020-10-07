#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "log.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


struct log_descriptor* log_init(void) {
  struct log_descriptor *ld = calloc(1, sizeof(struct log_descriptor) );
  ld->filepath = NULL;
  ld->fd = stdout;
  return ld;
}

struct log_descriptor* log_init2(const char *logpath) {
  struct log_descriptor *ld = calloc(1, sizeof(struct log_descriptor) );
  ld->filepath = calloc(1, strlen(logpath)+1);
  memcpy(ld->filepath, logpath, strlen(logpath) );
  ld->fd = fopen(ld->filepath, "a");
  if(ld->fd == NULL) {
    free(ld->filepath);
    free(ld);
    return NULL;
  }
  return ld;
}

void log_write(struct log_descriptor *ld, const char *data, size_t datasz) {
  time_t t;
  struct tm *ti;
  char *buffer;

  time(&t);
  ti = localtime(&t);
  asprintf(&buffer, "[%02d:%02d:%02d | %02d:%02d:%d] - ", ti->tm_sec, ti->tm_min,
                        ti->tm_hour,ti->tm_mday, ti->tm_mon, ti->tm_year+1900);
  fwrite(buffer, 1, strlen(buffer), ld->fd);
  fwrite(data, 1, datasz, ld->fd);
  fflush(ld->fd);
  free(buffer);
}

void log_free(struct log_descriptor *ld) {
  free(ld->filepath);
  fclose(ld->fd);
  free(ld);
}
