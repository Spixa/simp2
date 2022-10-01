#include "server.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
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
      simp::lex(tmp, credientials, '\x01');

      client->uname = credientials[0];
      spdlog::get("auth")->info(credientials[0] + " (" +
                                std::to_string(client.get()->get_id()) +
                                ") is authenticated");

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
      msg.set_content(Packets::SendMessagePacket,
                      {msg.decode_message(key)[0], client->uname}, key);
      broadcast(msg);

      spdlog::get("incoming")
          ->info(msg.decode_message(key)[1] + ": '" +
                 msg.decode_message(key)[0] + "'");
    }
  }
};

void setup_logger();
int handle_args(int argc, char** argv, uint16_t& port, std::string& version, std::string& name);

int main(int argc, char **argv) {
  setup_logger();

  uint16_t server_port;
  std::string name, version;
  
  int hand_args = handle_args(argc, argv, server_port, version, name);
  if (hand_args != 0) return hand_args;

  spdlog::info("Starting: " + name);

  Server s{server_port};
  s.start();

  while (true) {
    s.update();
  }
}

void setup_logger() {
  auto console = spdlog::stdout_color_mt("console");
  auto incoming = spdlog::stdout_color_mt("incoming");
  // incoming->set_pattern("%^Incoming%$ [%H:%M:%S %z] %v");

  auto auth = spdlog::stdout_color_mt("auth");
  spdlog::set_default_logger(console);
  spdlog::set_pattern("%^%L%$ [%H:%M:%S %z] (%n) > %v");
}

int handle_args(int argc, char** argv, uint16_t& port, std::string& version, std::string& name) {
  std::deque<std::string> args{argv, argv+argc};
  std::cout << "size is : " << args.size() << std::endl;

  if (args.size() <= 2) {
    spdlog::error("Usage: " + args[0] + " <run/create> <server.xml>");
    return -1;
  }

  if (args[1] == "create") {
    spdlog::error("Subcommand create is currently not implemented");
    return EXIT_FAILURE;
  } else if (args[1] == "run") { spdlog::info("Attempting run"); }
  else {
    spdlog::error("Subcommand " + args[1] + " not found");
    spdlog::error("Usage: " + args[0] + " <run/create> <server.xml>");
    return -10;
  }

  std::string path = args[2];
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
    spdlog::warn(path +
                 ": it is advised to add encryption node for next versions.");
  }

  port = std::atoi(port_node->ToElement()->GetText());
  version = protocol_version->ToElement()->GetText();
  name = server_name->ToElement()->GetText();

  spdlog::info("Checking profile's requested protocol version...");

  if (version != simp::OPENSIMP_VERSION) {
    spdlog::error("Protocol version requested (v" + version + ") is not supported by openSIMP protocol v" + simp::OPENSIMP_VERSION);
    
    if (version.find(".") != std::string::npos) {
      spdlog::warn("It seems the profile used decimal points for protocol version (.), it should be noted that the protocol versions are in integers");
    }
    return -8;
  }

  spdlog::info("Protocol version (v" + version + ") is supported!");

  return 0;
}