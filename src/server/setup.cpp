#include "setup.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <filesystem>
#include "../common/simp_protocol.hpp"

namespace fs = std::filesystem;

void setup_logger() {
  auto console = spdlog::stdout_color_mt("console");
  auto incoming = spdlog::stdout_color_mt("incoming");
  auto meta = spdlog::stdout_color_mt("meta");
  // incoming->set_pattern("%^Incoming%$ [%H:%M:%S %z] %v");

  auto auth = spdlog::stdout_color_mt("auth");
  spdlog::set_default_logger(console);
  spdlog::set_pattern("%^%L%$ [%H:%M:%S %z] (%n) > %v");
}

int parse(uint16_t& port, std::string& version, std::string& name) {
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

 int run(std::deque<std::string> args, uint16_t& port, std::string& version, std::string& name) {
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

    return 0;
 }

 int create(uint16_t& port, std::string& version, std::string& name, std::string n) {
    spdlog::warn("Custom server creation is not a feature in openSIMP v2 server binaries");
    spdlog::warn("Manually editing the server.xml file is suggested");
    tinyxml2::XMLDocument document;

    using namespace tinyxml2;

    spdlog::get("meta")->info("Welcome to simpserver_creator v2");

    XMLElement* root = document.NewElement("server");
    root->SetAttribute("generator", "basic_simp2");
    spdlog::get("meta")->info("Generated server node");

    document.InsertFirstChild(root);

    XMLElement* eName = document.NewElement("name");
    eName->SetText(n.data());
    root->InsertEndChild(eName);

    XMLElement* ePort = document.NewElement("port");
    ePort->SetText("37549");
    root->InsertEndChild(ePort);

    spdlog::get("meta")->info("Generated server port");

    XMLElement* eProtocol = document.NewElement("protocol");
    // eProtocol->SetText("\n\t\t");
    root->InsertEndChild(eProtocol);
    spdlog::get("meta")->info("Checking current protocol");

    XMLElement* eProtVersion = document.NewElement("version");
    eProtVersion->SetText("2");
    eProtocol->InsertEndChild(eProtVersion);

    spdlog::get("meta")->info("Created server database: ./" + n);
    XMLElement* eDatabase = document.NewElement("database");
    eDatabase->SetAttribute("dir", n.data());
    root->InsertEndChild(eDatabase);

    try {
      fs::create_directory(n);
      fs::create_directory(n + "/plugins");
      fs::create_directory(n + "/db");
    } catch (fs::filesystem_error e) {
      spdlog::get("meta")->error("Filesystem error: " + std::string(e.what()));
      exit(0);
    }

    spdlog::get("meta")->info("Enabled simpserver_v2 protocol for this server");

    spdlog::get("meta")->info("Successfully created default profile 'server.xml'");

    document.SaveFile((n + ".xml").data());
    port = 37549;
    version = "2";
    name = n;
    return 0;
 }


 /////////////////
int handle_args(int argc, char** argv, uint16_t& port, std::string& version, std::string& name) {
  std::deque<std::string> args{argv, argv+argc};
  if (args.size() <= 2) {
    spdlog::error("Usage: " + args[0] + " <run/create> <[name]>");
    return -1;
  }

  if (args[1] == "create") {
    spdlog::info("Creating " + args[2] + "...");
    if (create(port, version, name, args[2]) != 0) exit(EXIT_FAILURE);

  } else if (args[1] == "run") { spdlog::info("Attempting run"); if (run(args, port, version, name) != 0) exit(EXIT_FAILURE); }
  else {
    spdlog::error("Subcommand " + args[1] + " not found");
    spdlog::error("Usage: " + args[0] + " <run/create> <[name]>");
    return -10;
  }

  if (parse(port, version, name) != 0)
    return -8; 

  return 0;
}