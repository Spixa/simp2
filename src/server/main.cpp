#include "server.hpp"
#include <string>

#include <tinyxml2.h>

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
    simp::message<Packets> msg;
    msg.set_content(Packets::LeaveMessagePacket, {std::to_string(client->get_id())});

    broadcast(msg, client);
  }

  void onMessage(std::shared_ptr<connection<Packets>> client, message<Packets>& msg) override{
    if (msg.get_id() == Packets::SendMessagePacket) {
      msg.set_content(Packets::SendMessagePacket, {msg.decode_message()[0], std::to_string(client->get_id())});
      broadcast(msg);
      std::cout << msg.decode_message()[1] << ": '" << msg.decode_message()[0] << "'\n";
      
    }
  }

};

int main(int argc, char** argv) {
  if (argc == 1) {
    std::cerr << "Usage: " << argv[0] << " <server.xml>\n"; 
    return -1;
  }

  std::string path = argv[1];
  tinyxml2::XMLDocument document;
  tinyxml2::XMLError result = document.LoadFile(path.data());

  if (result != tinyxml2::XML_SUCCESS) {
    std::cerr << "An error occured while reading " << path << '\n';
    return -2;
  }

  tinyxml2::XMLNode* server = document.FirstChildElement("server");
  if (server == nullptr) {
    std::cerr << path << " doesn't contain node <server>\n";
    return -3;
  }

  tinyxml2::XMLNode* port_node = server->FirstChildElement("port");
  if (port_node == nullptr) {
    std::cerr << path << " doesn't contain node <port>\n";
    return -4;
  }

  uint16_t port = std::atoi(port_node->ToElement()->GetText());
  Server s{port};
  s.start();

  while (true) {
    s.update();
  }
}