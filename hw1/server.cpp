#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <thread>
#include "socket_tools.h"

void listenSocket(int sfd) {
  while (true) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
        printf("%s\n", buffer); // assume that buffer is a string
    }
  }
}

int main(int argc, const char **argv)
{
  const char *port = "2023", *recv_port = "2022";
  addrinfo resAddrInfo;
  int sfd = create_dgram_socket(nullptr, port, nullptr);
  int sfd_recv = create_dgram_socket("localhost", recv_port, &resAddrInfo);

  if (sfd == -1 || sfd_recv == -1) {
    return 1;
  }
  printf("listening!\n");
  std::thread thr(listenSocket, sfd);
  thr.detach();

  std::string input;
  while (true) { 
    printf(">");
    std::getline(std::cin, input);
    ssize_t res = sendto(sfd_recv, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
    if (res == -1) {
      std::cout << strerror(errno) << std::endl;
    }
  }
  return 0;
}