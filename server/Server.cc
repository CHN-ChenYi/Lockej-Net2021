#include <unistd.h>

#include <climits>
#include <cstring>

#include "../MsgDef.hpp"
#include "Pool.hpp"

int main() {
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  Pool pool(hostname);

  // create
  socket_fd sockfd;
  sockaddr_in serverAddr;
  if (~(sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
    // TODO
  }

  // bind
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(kServerPort);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serverAddr.sin_zero), 8);
  if (~bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) {
    // TODO
  }

  // listen
  if (~listen(sockfd, 5)) {
    // TODO
  }

  for (;;) {
    socket_fd comfd;
    sockaddr_in clientAddr;
    auto socketaddr_size = sizeof(sockaddr_in);
    if (~(comfd = accept(sockfd, reinterpret_cast<sockaddr *>(&clientAddr),
                         reinterpret_cast<socklen_t *>(&socketaddr_size)))) {
      // TODO
    }
    pool.AddClient(clientAddr, comfd);
  }

  return 0;
}
