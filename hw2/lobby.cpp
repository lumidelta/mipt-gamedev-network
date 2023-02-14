#include <enet/enet.h>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <cstring>

char server_port[] = "10888";

void send_port(ENetPeer *peer) {
  ENetPacket *packet = enet_packet_create(server_port, strlen(server_port) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}


int main(int argc, const char **argv)
{
  bool is_started = false;
  std::map<std::pair<unsigned, unsigned short>, ENetPeer *> active_connections;
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        active_connections[std::make_pair(event.peer->address.host, event.peer->address.port)] = event.peer;
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (is_started) {
          printf("Sending server port to new %x:%u\n", event.peer->address.host, event.peer->address.port);
          send_port(event.peer);
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (std::string((char *)(event.packet->data)) == "start") {
          printf("Special message recieved: '%s'\n", event.packet->data);
          for (auto const& x : active_connections) {
            printf("Sending server port to existing %x:%u\n", x.first.first, x.first.second);
            send_port(x.second);
          }
          is_started = true;
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}

