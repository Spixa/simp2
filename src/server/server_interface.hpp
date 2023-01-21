#pragma once
#include "server.hpp"
#include "server_connection.hpp"
#include "user_manager.hpp"
#include "cmd_processor.hpp"

#include "../common/message.hpp"
#include "../common/simp_protocol.hpp"

static constexpr const char *const key =
    "2B7E151628AED2A6ABF7158809CF4F3C2B7E151628AED2A6ABF7158809CF4F3C";

using namespace simp;

class Server : public simp::server_interface {
private:
  CommandProcessor cmdp;
public:
  Server(std::string const& name, uint16_t port) : simp::server_interface(port), user_manager_(name) {}

protected:
  bool onClientConnect(std::shared_ptr<connection<Packets>> client) override;

  void onClientDisconnect(std::shared_ptr<connection<Packets>> client) override;

  void onMessage(std::shared_ptr<connection<Packets>> client,
                 message<Packets> &msg) override;
private:
  simp::user_manager user_manager_;
};