#include <stdint.h>

#define PORT 80

#define ERROR_OK       0
#define ERROR_SOCKET  -1
#define ERROR_BIND    -2
#define ERROR_LISTEN  -3

struct Peer_info {
  uint32_t fd;
  uint16_t port;
  char *ip;
};

int socket_init(int timeout);
int socket_listen(int sfd, int queuesz);
struct Peer_info* socket_client_accept(int sfd);
void socket_client_release(struct Peer_info *pi);
