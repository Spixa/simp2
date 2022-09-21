#include "server.hpp"
#include <string>

using namespace simp;

class Server : public simp::server_interface {
public:
  Server(uint16_t port) : simp::server_interface(port) {}
protected:
  bool onClientConnect(std::shared_ptr<connection<Packets>> client) override {
    simp::message<Packets> msg;
    msg.set_content(Packets::JoinMessagePacket, {std::to_string(client->get_id())});
    broadcast(msg, client);

    return true;
  }

  void onClientDisconnect(std::shared_ptr<connection<Packets>> client) override {
  }

  void onMessage(std::shared_ptr<connection<Packets>> client, message<Packets>& msg) override{
    if (msg.get_id() == Packets::SendMessagePacket) {
      msg.set_content(Packets::SendMessagePacket, {msg.decode_message()[0], std::to_string(client->get_id())});
      broadcast(msg);
      std::cout << msg.decode_message()[1] << ": '" << msg.decode_message()[0] << "'\n";
      
    }
  }

};

int main() {
  Server s{37549};

  s.start();

  while (true) {
    s.update();
  }
}