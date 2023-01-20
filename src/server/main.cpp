#include "server_interface.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include "setup.hpp"

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

  Server s{server_port};
  s.start();

  while (true) {
    s.update();
  }
}