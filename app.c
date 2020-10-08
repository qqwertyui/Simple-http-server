#include <unistd.h>
#include <stdio.h>
#include "http.h"
#include "socket.h"
#include <string.h>

/*
  Error constants
*/
#define ERR_SUCCESS         0
#define ERR_WRONG_ARGUMENT  1
#define ERR_SOCKET_INIT     2
#define ERR_SOCKET_LISTEN   3


const char *ENTRY_MSG = "Waiting for incoming connections...\n";

int main(int argc, char **argv) {
  const char *document_root = NULL, *server_root = NULL;
  struct log_descriptor *ld = NULL; // getopt always assigns something to it

  int opt;
  while((opt = getopt(argc, argv, "d:s:l::") ) != -1) {
    switch(opt) {
      case 'd': {
        document_root = optarg;
        break;
      }
      case 's': {
        server_root = optarg;
        break;
      }
      case 'l': {
        if(optarg) { // if filename is given
          ld = log_init2(optarg);
        }
        if(ld == NULL) {
          fprintf(stdout, "failed to open log file, redirecting to stdout\n");
        }
        break;
      }
      case '?': { // options without necessary arguments end here (single colon)
        return ERR_WRONG_ARGUMENT;
      }
    }
  }

  /*
    If not log file is given, then redirect output to stdout
  */
  if(ld == NULL) {
    ld = log_init();
  }

  int sfd = socket_init(5000); // timeout interval in ms
  if(sfd < 0) {
    return ERR_SOCKET_INIT;
  }
  if(socket_listen(sfd, 5) ) {
    return ERR_SOCKET_LISTEN;
  }

  log_write(ld, ENTRY_MSG, strlen(ENTRY_MSG) );
  while(1) {
    struct Peer_info *pi = socket_client_accept(sfd);

    char new_conn[64] = { 0 };
    sprintf(new_conn, "New connection from %s:%d\n", pi->ip, pi->port);
    log_write(ld, (const char*)new_conn, strlen(new_conn) );

    http_init(document_root, server_root, ld);
    http_handle_request(pi->fd);
    http_free();

    socket_client_release(pi);
  }
  log_free(ld);
  close(sfd);
  return ERR_SUCCESS;
}
