#include <arpa/inet.h>
#include <unistd.h>

#include <climits>
#include <cstring>
#include <iostream>
#include <string>

#include "../MsgDef.hpp"
#include "Pool.hpp"

int main(int argc, char **argv) {
  int port = kServerPort;
  if (argc == 2) {
    int tmp = std::stoi(argv[1]);
    if (0 <= tmp && tmp <= 65535)
      port = tmp;
  }

  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  Pool pool(hostname);

  // create
  socket_fd sockfd;
  sockaddr_in serverAddr;
  if (!~(sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
    std::cout << "socket() failed! errno: " << errno << std::endl;
    return -1;
  }

  // bind
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serverAddr.sin_zero), 8);
  if (!~bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) {
    std::cout << "bind() failed! errno: " << errno << std::endl;
    close(sockfd);
    return -1;
  }

  // listen
  if (!~listen(sockfd, 5)) {
    std::cout << "listen() failed! errno: " << errno << std::endl;
    close(sockfd);
    return -1;
  }

  std::cout << "Listening on port " << port << std::endl;

  for (;;) {
    socket_fd comfd;
    sockaddr_in clientAddr;
    auto socketaddr_size = sizeof(sockaddr_in);
    if (!~(comfd = accept(sockfd, reinterpret_cast<sockaddr *>(&clientAddr),
                          reinterpret_cast<socklen_t *>(&socketaddr_size)))) {
      std::cout << "accept() failed! errno: " << errno << std::endl;
    }
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, addr, sizeof(addr));
    std::cout << addr << ':' << ntohs(clientAddr.sin_port) << " connected."
              << std::endl;
    pool.AddClient(clientAddr, comfd);
  }

  return 0;
}
