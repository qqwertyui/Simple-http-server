#include "socket.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#define SOCKLEN sizeof(struct sockaddr);

static struct sockaddr_in sock_data;

int socket_init(int timeout) {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sfd == -1) {
    perror("socket");
    return ERROR_SOCKET;
  }
  struct timeval to;
  to.tv_sec = timeout;
  to.tv_usec = 0;
  if(setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&to, sizeof(to) ) < 0)
    perror("setsockopt failed\n");

  /*
    The following call in not that necessary, but it may be left here (maybe
    I miss something)
  */
  if(setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&to, sizeof(to) ) < 0)
     perror("setsockopt failed\n");

  int reuseaddr = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr) );

  sock_data.sin_family = AF_INET;
  sock_data.sin_port = htons(PORT); // convert to big endian (network endian)
  sock_data.sin_addr.s_addr = INADDR_ANY;
  if(bind(sfd, (const struct sockaddr*)&sock_data, sizeof(struct sockaddr) ) != 0) {
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
  struct pollfd fds;
  memset(&fds, 0, sizeof(struct pollfd) );
  fds.fd = sfd;
  fds.events = POLLIN;
  fds.revents = POLLIN;
  if(poll(&fds, (nfds_t)1, -1) < 0) {
    perror("poll");
    return NULL;
  }

  struct sockaddr_in tmp;
  memset(&tmp, 0, sizeof(struct sockaddr_in) );
  socklen_t socklen = SOCKLEN;
  int pfd = accept(sfd, (struct sockaddr*)&tmp, &socklen);
  if(pfd == -1) {
    perror("accept");
    return NULL;
  }

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
