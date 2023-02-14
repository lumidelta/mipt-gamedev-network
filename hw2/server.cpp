#include <enet/enet.h>
#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <cstring>
#include <queue>

struct DataInfo {
  unsigned long id;
  std::string name;
  ENetPeer * peer;
};


void send_msg(ENetPeer *peer, const char * msg) {
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_msg_unreleable(ENetPeer *peer, const char * msg) {
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

int main(int argc, const char **argv)
{
  srand((unsigned)time(0)); 
  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastPingSendTime = timeStart;

  std::map<ENetPeer *, DataInfo> active_connections;
  std::map<ENetPeer *, std::queue<int>> ping;
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10888;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  std::hash<std::string> hash_str;
  unsigned long id;
  std::string name;
  int num;
  DataInfo data;
  ENetPeer * key;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        id = hash_str(std::to_string(event.peer->address.host)) + hash_str(std::to_string(event.peer->address.port));
        name = std::to_string(event.peer->address.host) + ":" + std::to_string(event.peer->address.port);
        data = DataInfo({id, name, event.peer}); 
        printf("Connection with %x:%u established, id:%lu, name: %s\n", event.peer->address.host, event.peer->address.port, id, name.c_str());
        for (auto const& x : active_connections) {
          printf("Sending new user to client %s\n", x.second.name.c_str());
          send_msg(x.second.peer, ("New player\nid: " + std::to_string(data.id) + "\nname: " + data.name).c_str());
          send_msg(data.peer, ("Existing player\nid: " + std::to_string(x.second.id) + "\nname: " + x.second.name).c_str());
        }
        active_connections[event.peer] = data;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        num = std::atoi((char *)event.packet->data);
        printf("Number received '%d'\n", num);
        key = event.peer;
        if (ping.find(key) == ping.end() || ping[key].size() < 2) {
          ping[key].push(num);
        }
        else {
          ping[key].pop();
          ping[key].push(num);
        }
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        active_connections.erase(event.peer);
        printf("Connection with %x:%u losted", event.peer->address.host, event.peer->address.port);
        break;
      default:
        break;
      };
    }
    if (!active_connections.empty()) {
      uint32_t curTime = enet_time_get();
      if (curTime - lastFragmentedSendTime > 1000) {
        for (auto const& x : active_connections) {
          lastFragmentedSendTime = curTime;
          send_msg(x.second.peer, std::to_string(curTime + rand()).c_str());
        }
      }
      if (curTime - lastPingSendTime > 10000) {
        for (auto info = ping.begin(); info != ping.end(); info++ ) {
          if (info->second.size() == 2) {
            int diff = info->second.back() - info->second.front();
            for (auto sendto = ping.begin(); sendto != ping.end(); sendto++ ) {
              send_msg_unreleable(sendto->first, ("player name:" + active_connections[info->first].name + " ping: " + std::to_string(diff)).c_str());
            }
          }
        }
        lastPingSendTime = curTime;
      }
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}

