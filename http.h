#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "log.h"

#define HTTP_SERVER_VERSION         "1.1"
#define HTTP_DEFAULT_DOCUMENT_ROOT  "/public_html/"
#define HTTP_DEFAULT_SERVER_ROOT    "/default/"

#define INIT_BUFFER_SIZE 1024


// http methods
#define HTTP_METHOD_UNKNOWN   0
#define HTTP_METHOD_GET       1
#define HTTP_METHOD_HEAD      2

// protocol versions
#define HTTP_VERSION_UNKNOWN  1
#define HTTP_VERSION_1_1      1

struct Http_header {
  char *key;    // Content-Length
  char *value;  // 22

  char *field;  // Content-Length: 22
  size_t fieldsz;
};

struct Http_data {
  unsigned char *data;
  size_t datasz;
};

struct Http_container_node {
  struct Http_container_node *next;
  struct Http_header *cur;
};

struct Http_container {
  struct Http_container_node *head;
  size_t elements;
  size_t contentsz;
};

struct Http_env_config {
  char *rootdir;
  size_t rootdirsz;
  char *specialdir;
  size_t specialdirsz;
  struct log_descriptor *ld;
} http_config;

struct Http_request {
  uint8_t method;
  char *path;
  uint8_t version;
};

const char *response;

void http_init(const char *rootdir, const char *specialdir, struct log_descriptor *ld);
void http_free(void);
void http_handle_request(int pfd);
