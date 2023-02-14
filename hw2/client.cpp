#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <string>

void send_msg(ENetPeer *peer, const char * msg) {
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_start(ENetPeer *peer) {
  const char *msg = "start";
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void start_session(ENetPeer *peer) {
  while (true) {
    std::string input;
    std::cin >> input;
    if (input == "start") {
        const char *msg = "start";
      ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(peer, 0, packet);
    }
  }
}

int main(int argc, const char **argv) {
  srand((unsigned)time(0)); 
  int port = 0;
  int rand_num = rand();
  if (enet_initialize() != 0) {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client) {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10887;

  ENetPeer *serverPeer = nullptr;
  ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
  if (!lobbyPeer) {
    printf("Cannot connect to lobby");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool connected = false;
  std::thread thr(start_session, lobbyPeer);
  thr.detach();
  while (true) {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0) {
      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with lobby %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (event.peer == lobbyPeer && !connected) {
          port = std::atoi((char *)event.packet->data);
          printf("Port received '%d'\n", port);
          enet_packet_destroy(event.packet);

          ENetAddress address_server;
          enet_address_set_host(&address_server, "localhost");
          address_server.port = port;

          serverPeer = enet_host_connect(client, &address_server, 2, 0);
          if (!serverPeer)
          {
            printf("Cannot connect to server");
            return 1;
          }
          connected = true;
        }
        else {
          printf("Packet received '%s'\n", event.packet->data);
          enet_packet_destroy(event.packet);
        }
        break;
      default:
        break;
      };
    }
    if (connected) {
      uint32_t curTime = enet_time_get();
      if (curTime - lastFragmentedSendTime > 1000) {
        lastFragmentedSendTime = curTime;
        send_msg(serverPeer, std::to_string(curTime + rand_num).c_str());
      }
    }
  }
  return 0;
}
