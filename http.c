#include <linux/limits.h>
#include "http.h"
#include "utils.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include "mime.h"


const char *internal_error = "\
<html>\
  <head>\
    <title>500 Error</title>\
  </head>\
\
  <body>\
    <h1>Internal server error</h1>\
  </body>\
</html>\
";

/*
  Look at http_add_root for more info.
  HTTP_NORMAL means usual file (e.g. index.html)
  HTTP_SPECIAL means special file (e.g. error.html which is shown when 404 error occurs)
*/
#define HTTP_SPECIAL  0
#define HTTP_NORMAL   1

/*
  Look at http_parse_request for more info.
  INDEX_HTML constant is used for expanding default '/' path to 'index.html' one,
  and INDEX_HTML_LENGTH is necessary for memory operations (e.g. memcpy requires data size in bytes)
*/
const char *INDEX_HTML = "index.html";
#define INDEX_HTML_LENGTH strlen(INDEX_HTML)
const char *HTTP_PAGE_UNKNOWN = "unknown.html";

/*
  Here are defined status codes for repsonses (200 OK, 404 NOT FOUND and others)
*/
const char *HTTP_STATUS_404 = "404 Not Found";
const char *HTTP_STATUS_200 = "200 Ok";
const char *HTTP_STATUS_500 = "500 Internal Server Error";


const char *crlf = "\r\n";
const char *crlf2 = "\r\n\r\n";

#define __pagebufsz 10
static char __pagebuf[__pagebufsz];

static struct Http_header* http_create_header_2(const char *key);
static struct Http_header* http_create_header(const char *key, const char *value);

/*
  Checks if data stream is over
*/
static bool is_crlf(uint8_t byte, uint8_t *state) {
  if(byte == '\r') {
    if(*state == 0) {
      *state = 1;
    } else if(*state == 2) {
      *state = 3;
    } else {
      *state = 0;
    }
  }
  else if(byte == '\n') {
    if(*state == 1) {
      *state = 2;
    } else if(*state == 3) {
      return true;
    } else {
      *state = 0;
    }
  }
  return false;
}


/*
  Creates new container and returns a pointer to it
*/
static struct Http_container* http_container_init(void) {
  struct Http_container *hcont = calloc(1, sizeof(struct Http_container) );
  hcont->head = NULL; // assign NULL explicitly (not necessary at all after calloc but ok)
  hcont->elements = 0; // same as above
  hcont->contentsz = 0;
  return hcont;
}


/*
  Returns idx element of linked list. If there is no idx, returns NULL (indexing starts at 0)
*/
static struct Http_container_node* http_container_node_at(struct Http_container *hcont, size_t idx) {
  if(hcont->elements < idx) {
    return NULL;
  }
  struct Http_container_node *node = hcont->head;
  for(size_t i=0; i<idx; i++) {
    node = node->next;
  }
  return node;
}


/*
  Simplified version of above function; returns data in more comfortable form
*/
static struct Http_header* http_container_at(struct Http_container *hcont, size_t idx) {
  struct Http_container_node *tmp;
  return ((tmp = http_container_node_at(hcont, idx)) == NULL) ? NULL : tmp->cur;
}


/*
  Creates new element at the end of the list
*/
static void http_container_push_back(struct Http_container *hcont, const char *key, const char *value) {
  struct Http_container_node *last;
  if(hcont->elements == 0) {
    last = hcont->head = calloc(1, sizeof(struct Http_container_node) );
  }
  else {
    last = http_container_node_at(hcont, hcont->elements-1);
    last->next = calloc(1, sizeof(struct Http_container_node) );
    last = last->next;
  }

  last->next = NULL;
  last->cur = (value) ? http_create_header(key, value) : http_create_header_2(key);
  hcont->contentsz += last->cur->fieldsz;
  hcont->elements++;
}


/*
  Pops last element of the list, returns true on success and false on failure
*/
/*
static bool http_container_pop_back(struct Http_container *hcont) {
  if(hcont->elements == 0) {
    return false;
  }
  struct Http_container_node *penultimate = http_container_node_at(hcont, hcont->elements-2);
  if(penultimate) {
    hcont->contentsz -= penultimate->next->cur->fieldsz;
    free(penultimate->next->cur);
    free(penultimate->next);
    penultimate->next = NULL;
  }
  else {
    hcont->contentsz -= hcont->head->cur->fieldsz;
    free(hcont->head->cur);
    free(hcont->head);
    hcont->head = NULL;
  }

  hcont->elements--;
  return true;
}
*/

/*
  Frees whole list (don't use containter after freeing it)
*/
static void http_container_free(struct Http_container *hcont) {
  if(hcont->elements > 0) {
    struct Http_container_node *next, *cur = hcont->head;
    for(size_t i=0; i<hcont->elements; i++) {
      next = cur->next;
      free(cur->cur);
      free(cur);
      cur = next;
    }
  }
  memset(hcont, 0, sizeof(struct Http_container) );
  free(hcont);
}


/*
  Creates http header in format {key : value} and adds crlf at the end
*/
static struct Http_header* http_create_header(const char *key, const char *value) {
  const char *separator = ": ";
  struct Http_header *hdr = calloc(1, sizeof(struct Http_header) );

  size_t keylen = strlen(key);
  size_t valuelen = strlen(value);

  hdr->key = calloc(1, keylen);
  memcpy(hdr->key, key, keylen); // copy without '\0' byte
  hdr->value = calloc(1, valuelen); // same as above
  memcpy(hdr->value, value, valuelen);

  // add colon, space and crlf to total size (4 bytes)
  hdr->fieldsz = keylen+valuelen+4;
  hdr->field = calloc(1, hdr->fieldsz);

  size_t offset = 0;
  memcpy(hdr->field+offset, key, keylen); offset += keylen;
  memcpy(hdr->field+offset, separator, 2); offset += 2;
  memcpy(hdr->field+offset, value, valuelen); offset += valuelen;
  memcpy(hdr->field+offset, crlf, 2);

  return hdr;
}

/*
  Alternative version of http_create_header with one argment
  (Used in headers which are not in format {key:value} but {key} )
*/
static struct Http_header* http_create_header_2(const char *key) {
  struct Http_header *hdr = calloc(1, sizeof(struct Http_header) );
  size_t keylen = strlen(key);

  hdr->key = calloc(1, keylen);
  memcpy(hdr->key, key, keylen);

  hdr->fieldsz = keylen + 2; // crlf
  hdr->field = calloc(1, hdr->fieldsz);
  memcpy(hdr->field, key, keylen);
  memcpy(hdr->field+keylen, crlf, 2);

  return hdr;
}


/*
  Creates http response (also called http segment because it is tcp based protocol)
*/
static struct Http_data* http_create_response(struct Http_container *hcont, struct Http_data *hbody) {
  struct Http_data *response = calloc(1, sizeof(struct Http_data) );

  size_t total_size = hcont->contentsz + hbody->datasz;
  response->data = calloc(1, total_size);
  response->datasz = total_size;
  size_t offset = 0;
  for(size_t i=0; i<hcont->elements; i++) {
    struct Http_header *hdr = http_container_at(hcont, i);
    memcpy(response->data+offset, hdr->field, hdr->fieldsz);
    offset += hdr->fieldsz;
  }
  memcpy(response->data+offset, hbody->data, hbody->datasz);

  return response;
}


/*
  Recieves data from client and returns it along with its size
*/
static struct Http_data* http_recieve_request(int pfd) {
  struct Http_data *hr = calloc(1, sizeof(struct Http_data) );

  uint8_t rbuf, crlf_state = 0;
  int bufcngd = 1;

  int status = 0;
  for(int i=0; (status = recv(pfd, &rbuf, sizeof(uint8_t), 0) ) != 0; i++) {
    if(status == -1) {
      free(hr);
      return NULL;
    }
    printf("status->%d, idx->%d\n", status, i);
    if((i % INIT_BUFFER_SIZE) == 0) {
      hr->data = realloc(hr->data, bufcngd*INIT_BUFFER_SIZE);
      bufcngd++;
    }
    hr->data[i] = rbuf;
    if(is_crlf(rbuf, &crlf_state) ) {
      hr->datasz = i+1;
      break;
    }
  }
  return hr;
}


/*
  Returns path with rootdir appended at the beginning, free after use.
  Dirsz must exactly match string length (cannot have free space at the end)
*/
static char* http_add_root(const char *path, int mode) {
  size_t dirsz, pathsz = strlen(path);
  char *dir;
  if(mode == HTTP_NORMAL) {
    dirsz = http_config.rootdirsz;
    dir = http_config.rootdir;
  }
  else {
    dirsz = http_config.specialdirsz;
    dir = http_config.specialdir;
  }

  char *rootpath = calloc(1, pathsz+dirsz+1);
  memcpy(rootpath, dir, dirsz);
  memcpy(rootpath+dirsz, path, pathsz);
  return rootpath;
}


static struct Http_data* http_read_file(char *path) {
  struct Http_data *hdata = calloc(1, sizeof(struct Http_data) );
  FILE *f = fopen(path, "rb");
  if(!f) {
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  hdata->datasz = ftell(f);
  hdata->data = calloc(1, hdata->datasz);
  rewind(f);
  fread(hdata->data, 1, hdata->datasz, f);
  if(ferror(f) ) {
    return NULL;
  }
  fclose(f);
  return hdata;
}


/*
  Reads resource requested by user and returns a pointer to it
*/
static struct Http_data* http_resource_read(const char *path, int mode) {
  struct Http_data *hdata = calloc(1, sizeof(struct Http_data) );
  char *full_path = http_add_root(path, mode);
  hdata = http_read_file(full_path);
  free(full_path);
  return hdata;
}


/*
  Check the existence of resource at rootdir + path
*/
static bool http_resource_is_avalible(char *path) {
  char *full_path = http_add_root(path, HTTP_NORMAL);
  bool status = false;

  struct stat st;
  memset(&st, 0, sizeof(struct stat) );
  stat(full_path, &st);
  // check if file is directory, link or whatever (not regular file)
  if((st.st_mode & S_IFMT) == S_IFREG) {
    FILE *f = fopen(full_path, "rb");
    status = (f) ? true : false;
    if(status) fclose(f);
    free(full_path);
  }
  return status;
}


/*
  Frees data held in Http_data structures
*/
static void http_free_data(struct Http_data *resource) {
  free(resource->data);
  free(resource);
}


/*
  Sets mime type for given file
*/
static void http_set_content(struct Http_container *hheaders, struct Http_data *hbody, const char *filepath) {
  http_container_push_back(hheaders, "Content-Type", mime_get_type(filepath) );
  http_container_push_back(hheaders, "Content-Length", __uitoa(hbody->datasz) );
}

static void http_set_status(struct Http_container *hheaders, const char *status) {
  char full_status[50] = { "HTTP/" };
  size_t offset = 5;

  memcpy(full_status+offset, HTTP_SERVER_VERSION, 3); offset += 3; // "1.1" -> 3 characters
  memcpy(full_status+offset, " ", 1); offset += 1; // insert space
  memcpy(full_status+offset, status, strlen(status) );
  http_container_push_back(hheaders, full_status, NULL);
}

static bool http_is_default(char *path) {
  return (memcmp(path, "/ ", 2) == 0) ? true : false;
}

static uint8_t http_get_method(char *data) {
  if(!memcmp(data, "GET", 3) ) {
    return HTTP_METHOD_GET;
  } else if(!memcmp(data, "HEAD", 4) ) {
    return HTTP_METHOD_HEAD;
  }
  return HTTP_METHOD_UNKNOWN;
}

static uint8_t http_get_version(char *data) {
  data += 5;
  /*
    data should look like: "HTTP/1.1"
    and necessary part is only "1.1"
    so move pointer by 5 bytes forth
  */
  if(!memcmp(data, "1.1", 3) ) {
    return HTTP_VERSION_1_1;
  }
  return HTTP_VERSION_UNKNOWN;
}

static char* http_get_path(char *data) {
  // input data looks like "/index.html"
  char *path, *result;
  size_t pathsz;

  if(http_is_default(data) ) { // '/' directory
    pathsz = INDEX_HTML_LENGTH;
    path = (char*)INDEX_HTML;
  } else {
    path = data;
    pathsz = strchr(data, ' ') - data;
  }
  result = calloc(1, pathsz+1); // +1 to make it null terminated string (can be printed later)
  memcpy(result, path, pathsz);
  return result;
}

/*
  Parse http request (split request into smaller pieces and read metadata).
  The path it returns is absolute to current root directory
*/
static struct Http_request* http_parse_request(struct Http_data *hreqd) {
  struct Http_request *hreq = calloc(1, sizeof(struct Http_request) );

  char *method = (char*)hreqd->data;
  char *path = strchr((char*)hreqd->data, ' ')+1; // +2 to ommit space and slash at the beginning
  char *version = strchr(path, ' ')+1; // ommit space

  hreq->method = http_get_method(method);
  hreq->path = http_get_path(path);
  hreq->version = http_get_version(version);
  return hreq;
}

static char* http_status_to_page(const char *status) {
  memset(__pagebuf, 0, __pagebufsz);
  memcpy(__pagebuf, status, 3);
  memcpy(__pagebuf+3, ".html", 6); // status codes are up to 3 characters
  return __pagebuf;
}

static bool http_status_equals(const char *status1, const char *status2) {
  return (strcmp(status1, status2) == 0) ? true : false;
}

static struct Http_data* http_read_internal_resource(const char *ptr) {
  struct Http_data *hdata = calloc(1, sizeof(struct Http_data) );
  hdata->datasz = strlen(ptr)+1;
  hdata->data = calloc(1, hdata->datasz);
  memcpy(hdata->data, ptr, hdata->datasz);
  return hdata;
}

/*
  Initializes root directory for web server
*/
void http_init(const char *rootdir, const char *specialdir, struct log_descriptor *ld) {
  memset(&http_config, 0, sizeof(struct Http_env_config) );

  http_config.ld = ld;
  if(rootdir) {
    http_config.rootdirsz = strlen(rootdir)+1;
    http_config.rootdir = calloc(1, http_config.rootdirsz);
    memcpy(http_config.rootdir, rootdir, http_config.rootdirsz);
  } else {
    http_config.rootdir = calloc(1, PATH_MAX);
    getcwd(http_config.rootdir, PATH_MAX);
    http_config.rootdirsz = strlen(http_config.rootdir)+strlen(HTTP_DEFAULT_DOCUMENT_ROOT);
    strcat(http_config.rootdir, HTTP_DEFAULT_DOCUMENT_ROOT);
  }

  if(specialdir) {
    http_config.specialdirsz = strlen(specialdir)+1;
    http_config.specialdir = calloc(1, http_config.specialdirsz);
    memcpy(http_config.specialdir, specialdir, http_config.specialdirsz);
  } else {
    http_config.specialdir = calloc(1, PATH_MAX);
    getcwd(http_config.specialdir, PATH_MAX);
    http_config.specialdirsz = strlen(http_config.specialdir)+strlen(HTTP_DEFAULT_SERVER_ROOT);
    strcat(http_config.specialdir, HTTP_DEFAULT_SERVER_ROOT);
  }
}


/*
  Use if http_init was used before
*/
void http_free(void) {
  free(http_config.rootdir);
  free(http_config.specialdir);
}


/*
  Routine which checks if all paths are well placed.
*/
static bool http_check_setup(void) {
  struct stat st;
  memset(&st, 0, sizeof(struct stat) );
  stat(http_config.rootdir, &st);
  if(S_ISDIR(st.st_mode) ) { // check if http_config.rootdir exist
    stat(http_config.specialdir, &st);
    if(S_ISDIR(st.st_mode) ) { // check if http_config.specialdir exist
      return true;
    }
  }
  return false;
}


static bool http_request_is_empty(struct Http_data *hdata) {
  if(hdata == NULL) return true;
  return (hdata->datasz) ? false : true;
}

/*
  Main http routine
*/
void http_handle_request(int pfd) {
  struct Http_container *hheaders = http_container_init();

  struct Http_data *hreqd = http_recieve_request(pfd);
  if(http_request_is_empty(hreqd) ) {
    http_container_free(hheaders);
    if(hreqd) {
      http_free_data(hreqd);
    }
    return;
  }
  log_write(http_config.ld, (const char*)hreqd->data, hreqd->datasz);
  struct Http_request *hreq = http_parse_request(hreqd);

  struct Http_data *hbody;
  const char *page_path, *status = NULL;
  if(!http_check_setup() ) {
    status = HTTP_STATUS_500;
    hbody = http_read_internal_resource(internal_error);
    page_path = http_status_to_page(status);
  } else if(http_resource_is_avalible(hreq->path) ) { // if any is set
    status = HTTP_STATUS_200;
    page_path = hreq->path;
  } else {
    status = HTTP_STATUS_404;
    page_path = http_status_to_page(status);
  }
  // Read response body (only when status is different from 500)
  if(!http_status_equals(status, HTTP_STATUS_500) ) {
    hbody = (http_status_equals(status, HTTP_STATUS_200) ) ? http_resource_read(page_path, HTTP_NORMAL)
      : http_resource_read(page_path, HTTP_SPECIAL);
  }

  http_set_status(hheaders, status);
  http_set_content(hheaders, hbody, page_path);

  // Set Connection header and append crlf ending
  http_container_push_back(hheaders, "Connection", "close\r\n");

  struct Http_data *hresp = http_create_response(hheaders, hbody);
  write(pfd, hresp->data, hresp->datasz);

  http_free_data(hresp);
  http_free_data(hbody);
  http_container_free(hheaders);
  http_free_data(hreqd);
}
