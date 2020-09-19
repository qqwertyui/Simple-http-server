#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

#define PORT 80

int main(int argc, char **argv) {
  if(argc != 2) {
    puts("Usage: ./client <data>");
    return 1;
  }
  const char *crlf = "\r\n\r\n";
  struct sockaddr_in sock_data;

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sfd == -1) {
    perror("socket");
    return 1;
  }

  memset((void*)&sock_data, 0, sizeof(struct sockaddr) );
  sock_data.sin_family = AF_INET;
  sock_data.sin_port = htons(PORT);
  sock_data.sin_addr.s_addr = inet_addr("127.0.0.1");
  connect(sfd, (struct sockaddr *)&sock_data, sizeof(sock_data) );

  int argv_len = strlen(argv[1] );
  int full_len = argv_len + 4;
  char *buffer = calloc(1, full_len);
  memcpy(buffer, argv[1], argv_len);
  memcpy(buffer+argv_len, crlf, 5);

  full_len += 4; // crlf
  write(sfd, buffer, full_len);

  memset(buffer, 0, full_len);
  read(sfd, buffer, full_len);
  write(1, buffer, full_len);

  close(sfd);
  return 0;
}
