#include <unistd.h>
#include <stdio.h>
#include "http.h"
#include "socket.h"


int main(int argc, char **argv) {
  const char *document_root = NULL;
  const char *server_root   = NULL;

  if(argc == 3) {
    document_root = argv[1];
    server_root   = argv[2];
  }

  int sfd = socket_init(5000); // timeout interval in ms
  if(sfd < 0) {
    return 2;
  }
  if(socket_listen(sfd, 5) ) {
    return 3;
  }
  puts("waiting for incoming connections...");

  while(1) {
    struct Peer_info *pi = socket_client_accept(sfd);

    printf("New connection from %s:%d\n", pi->ip, pi->port);
    http_init(document_root, server_root);
    http_handle_request(pi->fd);
    http_free();

    socket_client_release(pi);
  }
  close(sfd);
  return 0;
}
