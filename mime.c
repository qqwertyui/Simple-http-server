#include "mime.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *mime_extension[MIME_AMOUNT] = {
  "html;htm",
  "css",
  "js",
  "gz",
  "json",
  "jpg;jpeg",
  "gif",
  "png",
  "mp3",
  "wav"
};

const char *mime_type[MIME_AMOUNT] = {
  "text/html",
  "text/css",
  "text/javascript",
  "application/gzip",
  "application/json",
  "image/jpeg",
  "image/gif",
  "image/png",
  "audio/mpeg",
  "audio/wav"
};

const char *default_binary = "application/octet-stream";

#define __internal_mimebufsz 20

const char* mime_get_type(const char *filepath) {
  #warning add utf-8 encoding to some mimes
  char mimebuf[__internal_mimebufsz];
  char *user_ext = strrchr(filepath, '.');
  if(!user_ext) {
    return default_binary;
  }
  user_ext++; // move one character forth
  for(size_t i=0; i<MIME_AMOUNT; i++) {
    memset(mimebuf, 0, __internal_mimebufsz);
    memcpy(mimebuf, mime_extension[i], strlen(mime_extension[i] ) );
    char *mime_ext = mimebuf;
    size_t variants = 1;

    // overwrite semicolons with null bytes and count them
    char *tmp = strchr(mime_ext, ';');
    while(tmp) {
      *tmp = 0;
      variants++;
      tmp++;
      tmp = strchr(tmp, ';');
    }

    for(size_t j=0; j<variants; j++) {
      if(!strcmp(user_ext, mime_ext) ) {
        return mime_type[i];
      }
      mime_ext += strlen(mime_ext)+1;
    }
  }
  return default_binary;
}
