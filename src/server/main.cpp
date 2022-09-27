#include "server.hpp"
#include <spdlog/spdlog.h>
#include <string>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <tinyxml2.h>

using namespace simp;
static constexpr const char *const key =
    "2B7E151628AED2A6ABF7158809CF4F3C2B7E151628AED2A6ABF7158809CF4F3C";
class Server : public simp::server_interface {
public:
  Server(uint16_t port) : simp::server_interface(port) {}

protected:
  bool onClientConnect(std::shared_ptr<connection<Packets>> client) override {
    simp::message<Packets> msg;
    msg.set_content(Packets::JoinMessagePacket,
                    {std::to_string(client->get_id())}, key);
    broadcast(msg, client);

    std::stringstream oss;
    oss << decryptor.get()->public_key();

    const auto pubkey = oss.str();

    msg.body = std::vector<uint8_t>(pubkey.begin(), pubkey.end());
    msg.header.size = msg.body.size();

    message_client(client, msg);
    client->auth = connection<Packets>::AuthState::ModuloSent;

    return true;
  }

  void
  onClientDisconnect(std::shared_ptr<connection<Packets>> client) override {
    simp::message<Packets> msg;
    msg.set_content(Packets::LeaveMessagePacket,
                    {std::to_string(client->get_id())}, key);

    broadcast(msg, client);
  }

  void onMessage(std::shared_ptr<connection<Packets>> client,
                 message<Packets> &msg) override {
    if (client->auth == connection<Packets>::AuthState::ModuloSent) {
      secure_vector<uint8_t> cryptic_msg;

      for (auto x : msg.body) {
        cryptic_msg.push_back(x);
      }

      secure_vector<uint8_t> msg_body;
      try {
        msg_body = decryptor.get()->decrypt(cryptic_msg);
      } catch (Botan::Decoding_Error const &error) {
        spdlog::get("auth")->error("Couldn't decode auth info from client.");
        client->disconnect();
        return;
      }

      auto tmp = std::string(msg_body.begin(), msg_body.end());

      std::vector<std::string> credientials;
      text::engine::simplex(tmp, credientials, '\x01');

      client->uname = credientials[0];
      spdlog::get("auth")->info(credientials[0] + " (" + std::to_string(client.get()->get_id()) + ") is authenticated");

      client->auth = connection<Packets>::AuthState::AuthReceived;
      return;
    }

    if (client->auth == connection<Packets>::AuthState::AuthReceived) {
      BigInt pubkey = BigInt(std::string(
          reinterpret_cast<char *>(msg.body.data()), msg.header.size));
      encryptor.get()->new_key(client->get_id(), pubkey);
      std::cout << "\n";

      auto msg_body = encryptor.get()->encrypt(client->get_id(), key);
      simp::message<Packets> kmsg;

      for (auto x : msg_body) {
        kmsg.body.push_back(x);
      }

      kmsg.header.size = kmsg.body.size();
      message_client(client, kmsg);
      client->auth = connection<Packets>::AuthState::AesKeySent;

      return;
    }
    if (msg.get_id() == Packets::SendMessagePacket) {
      msg.set_content(
          Packets::SendMessagePacket,
          {msg.decode_message(key)[0], client->uname}, key);
      broadcast(msg);
      
      spdlog::get("incoming")->info(msg.decode_message(key)[1] + ": '"
                + msg.decode_message(key)[0] + "'");
    }
  }
};

int main(int argc, char **argv) {
  
  auto console = spdlog::stdout_color_mt("console");
  auto incoming = spdlog::stdout_color_mt("incoming");
  auto auth = spdlog::stdout_color_mt("auth");
  spdlog::set_default_logger(console);
  spdlog::set_pattern("%^%L%$ [%H:%M:%S %z] (%n) > %v");
  
  if (argc == 1) {
    spdlog::error("Usage: " + std::string(argv[0]) + " <server.xml>");
    return -1;
  }

  std::string path = argv[1];
  tinyxml2::XMLDocument document;
  tinyxml2::XMLError result = document.LoadFile(path.data());

  spdlog::info("Reading profile...");

  if (result != tinyxml2::XML_SUCCESS) {
    spdlog::error("An error occured while reading " + path);
    return -2;
  }

  tinyxml2::XMLNode *server = document.FirstChildElement("server");
  if (server == nullptr) {
    spdlog::error(path + " doesn't contain node <server>");
    return -3;
  }

  tinyxml2::XMLNode *port_node = server->FirstChildElement("port");
  if (port_node == nullptr) {
    spdlog::error(path + " doesn't contain node <port>");
    return -4;
  }

  tinyxml2::XMLNode *server_name = server->FirstChildElement("name");
  if (server_name == nullptr) {
    spdlog::error(path + " doesn't contain node <name>");
    return -5;
  }

  tinyxml2::XMLNode *protocol = server->FirstChildElement("protocol");
  if (protocol == nullptr) {
    spdlog::error(path + " does not define protocol");
    return -6;
  }

  tinyxml2::XMLNode *protocol_version = protocol->FirstChildElement("version");
  if (protocol_version == nullptr) {
    spdlog::error(path + " does not define protocol version");
    return -7;
  }

  tinyxml2::XMLNode *encryption_mode =
      protocol->FirstChildElement("encryption");
  if (encryption_mode == nullptr) {
    spdlog::warn(path
              + ": it is advised to add encryption node for next versions.");
  }

  uint16_t port = std::atoi(port_node->ToElement()->GetText());
  std::string v = protocol_version->ToElement()->GetText();
  std::string n = server_name->ToElement()->GetText();

  spdlog::info("Starting server...");
  Server s{port};
  s.start();

  while (true) {
    s.update();
  }
}
