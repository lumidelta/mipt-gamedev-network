#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <unistd.h>
#include "socket_tools.h"

void keepAlive(int sfd, addrinfo resAddrInfo) {
  while (true) {
    sleep(10);
    std::string message = "I'm alive";
    ssize_t res = sendto(sfd, message.c_str(), message.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
    // std::cout << "Sending alive message: " << message << std::endl;
    if (res == -1) {
      std::cout << strerror(errno) << std::endl;
    }
  }
}

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

int main(int argc, const char **argv) {
  const char *port = "2023", *recv_port = "2022";
  std::string input;
  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);
  int sfd_recv = create_dgram_socket(nullptr, recv_port, nullptr);


  if (sfd == -1 || sfd_recv == -1) {
    printf("Cannot create a socket\n");
    return 1;
  }

  srand((unsigned)time(0));
  input = std::to_string(std::rand());
  ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
  std::cout << "Sending init number: " << input << std::endl;
  if (res == -1) {
    std::cout << strerror(errno) << std::endl;
  }

  std::thread thr1(keepAlive, sfd, resAddrInfo);
  std::thread thr2(listenSocket, sfd_recv);
  thr1.detach();
  thr2.detach();
  
  while (true) { 
    printf(">");
    std::getline(std::cin, input);
    ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
    if (res == -1) {
      std::cout << strerror(errno) << std::endl;
    }
  }
  return 0;
}