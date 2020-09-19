#include "socket.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define SOCKLEN sizeof(struct sockaddr);

static struct sockaddr_in sock_data;

int socket_init(int timeout) {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sfd == -1) {
    perror("socket");
    return ERROR_SOCKET;
  }
  int reuseaddr = 1;
  setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout) );
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr) );

  sock_data.sin_family = AF_INET;
  sock_data.sin_port = htons(PORT); // convert to big endian (network endian)
  sock_data.sin_addr.s_addr = INADDR_ANY;
  if(bind(sfd, (const struct sockaddr*)&sock_data, sizeof(struct sockaddr) ) == -1) {
    perror("bind");
    return ERROR_BIND;
  }
  return sfd;
}

int socket_listen(int sfd, int queuesz) {
  if(listen(sfd, queuesz) ) {
    perror("listen");
    return ERROR_LISTEN;
  }
  return ERROR_OK;
}

struct Peer_info* socket_client_accept(int sfd) {
  struct sockaddr_in tmp;
  memset(&tmp, 0, sizeof(struct sockaddr_in) );
  socklen_t socklen = SOCKLEN;
  int pfd = accept(sfd, (struct sockaddr*)&tmp, &socklen);

  struct Peer_info *pi = calloc(1, sizeof(struct Peer_info) );
  pi->fd = pfd;
  pi->port = htons(tmp.sin_port);
  pi->ip = inet_ntoa(tmp.sin_addr);

  return pi;
}

void socket_client_release(struct Peer_info* pi) {
  close(pi->fd);
  free(pi);
}
