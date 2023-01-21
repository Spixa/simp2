#include "server_interface.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include "setup.hpp"

void startup_fail(std::exception const& e);
void server_fail(std::exception const& e);

int main(int argc, char **argv) {
  setup_logger();

  uint16_t server_port;
  std::string name, version;
  
  int hand_args = handle_args(argc, argv, server_port, version, name);
  if (hand_args != 0) return hand_args;

  spdlog::info("Starting: " + name);
  spdlog::info("openSIMP (core_v2, api_v2): Open Secure Instant Messaging Protocol");
  #ifdef __unix__
    spdlog::info("openSIMP (core_v2, api_v2): openSIMP v2 server for Unix/Linux: The secure IM protocol");
  #elif defined(WIN32)
    spdlog::info("openSIMP (core_v2, api_v2): openSIMP v2 server for Microsoft Windows: The secure IM protocol");
    spdlog::warn("You are using an unrecommended and unsafe operating system (Microsoft Windows-any). Try using GNU/Linux");
  #endif

  Server s{name, server_port};

  try {
    s.start();
  } catch (std::exception const& e) {
    startup_fail(e);
    exit(0);
  }

  try {
    while (true) {
      s.update();
    }
  } catch (std::exception const& e) {
    server_fail(e);
    exit(0);
  }
}

void startup_fail(std::exception const& e) {
  spdlog::error("An startup error kept the server from launching");
  spdlog::error(e.what());
}


void server_fail(std::exception const& e) {
  spdlog::error("A server exception caused the server to fatally crash");
  spdlog::error(e.what());
}
