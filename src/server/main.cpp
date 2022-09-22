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
    msg.set_content(Packets::JoinMessagePacket,
                    {std::to_string(client->get_id())});
    broadcast(msg, client);

    return true;
  }

  void
  onClientDisconnect(std::shared_ptr<connection<Packets>> client) override {
    simp::message<Packets> msg;
    msg.set_content(Packets::LeaveMessagePacket,
                    {std::to_string(client->get_id())});

    broadcast(msg, client);
  }

  void onMessage(std::shared_ptr<connection<Packets>> client,
                 message<Packets> &msg) override {
    if (msg.get_id() == Packets::SendMessagePacket) {
      msg.set_content(
          Packets::SendMessagePacket,
          {msg.decode_message()[0], std::to_string(client->get_id())});
      broadcast(msg);
      std::cout << msg.decode_message()[1] << ": '" << msg.decode_message()[0]
                << "'\n";
    }
  }
};

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "Usage: " << argv[0] << " <server.xml>\n";
    return -1;
  }

  std::string path = argv[1];
  tinyxml2::XMLDocument document;
  tinyxml2::XMLError result = document.LoadFile(path.data());

  std::cout << "Reading profile...\n";

  if (result != tinyxml2::XML_SUCCESS) {
    std::cerr << "An error occured while reading " << path << '\n';
    return -2;
  }

  tinyxml2::XMLNode *server = document.FirstChildElement("server");
  if (server == nullptr) {
    std::cerr << path << " doesn't contain node <server>\n";
    return -3;
  }

  tinyxml2::XMLNode *port_node = server->FirstChildElement("port");
  if (port_node == nullptr) {
    std::cerr << path << " doesn't contain node <port>\n";
    return -4;
  }

  tinyxml2::XMLNode *server_name = server->FirstChildElement("name");
  if (server_name == nullptr) {
    std::cerr << path << " doesn't contain node <name>\n";
    return -5;
  }

  tinyxml2::XMLNode *protocol = server->FirstChildElement("protocol");
  if (protocol == nullptr) {
    std::cerr << path << " does not define protocol\n";
    return -6;
  }

  tinyxml2::XMLNode *protocol_version = protocol->FirstChildElement("version");
  if (protocol_version == nullptr) {
    std::cerr << path << " does not define protocol version\n";
    return -7;
  }

  tinyxml2::XMLNode *encryption_mode =
      protocol->FirstChildElement("encryption");
  if (encryption_mode == nullptr) {
    std::cerr << path
              << ": it is advised to add encryption node for next versions.\n";
  }

  uint16_t port = std::atoi(port_node->ToElement()->GetText());
  std::string v = protocol_version->ToElement()->GetText();
  std::string n = server_name->ToElement()->GetText();

  std::cout << "Loading server...\n";
  Server s{port};
  s.start();
  std::cout << "\tServer:           " << n << "\n";
  std::cout << "\tPort:             " << int(port) << "\n";
  std::cout << "\tProtocol version: " << v << "\n";

  while (true) {
    s.update();
  }
}
