#include <stddef.h>
#include <stdio.h>

struct log_descriptor {
  char *filepath;
  FILE *fd;
};

struct log_descriptor* log_init(void);
struct log_descriptor* log_init2(const char *logpath);
void log_write(struct log_descriptor *ld, const char *data, size_t datasz);
void log_free(struct log_descriptor *ld);
