#include "server_interface.hpp"
#include <memory>

bool Server::onClientConnect(std::shared_ptr<connection<Packets>> client) {
    simp::message<Packets> msg;
    // msg.set_content(Packets::JoinMessagePacket,
    //                 {std::to_string(client->get_id())}, key);
    // broadcast(msg, client);

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
Server::onClientDisconnect(std::shared_ptr<connection<Packets>> client) {
    simp::message<Packets> msg;

    if (client->uname == "")
    spdlog::get("auth")->warn("Unauthenticated client left (" + std::to_string(client->get_id()) + ")");
    else {
    msg.set_content(Packets::LeaveMessagePacket,
                {client->uname}, key);
    spdlog::get("incoming")->info(client->uname + " disconnected");
    }
    broadcast(msg, client);
}

void Server::onMessage(std::shared_ptr<connection<Packets>> client,
                 message<Packets> &msg) {
    simp::message<Packets> orig = msg;
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
                                ") is authenticated using secure RSA encryption");

      msg.set_content(Packets::JoinMessagePacket,
                    {client->uname}, key);
      broadcast(msg, client);

      client->auth = connection<Packets>::AuthState::AuthReceived;
      return;
    }

    if (client->auth == connection<Packets>::AuthState::AuthReceived) {
      BigInt pubkey = BigInt(std::string(
          reinterpret_cast<char *>(msg.body.data()), msg.header.size));
      encryptor.get()->new_key(client->get_id(), pubkey);

      auto msg_body = encryptor.get()->encrypt(client->get_id(), key);
      simp::message<Packets> kmsg;

      for (auto x : msg_body) {
        kmsg.body.push_back(x);
      }

      kmsg.header.size = kmsg.body.size();
      message_client(client, kmsg);
      spdlog::get("auth")->info(client->uname + " (" +
                            std::to_string(client.get()->get_id()) +
                            ") is now able to communicate with this openSIMPv2 server");

      client->auth = connection<Packets>::AuthState::AesKeySent;

      return;
    }

    spdlog::debug(client->uname + " packet: {}", static_cast<uint32_t>(orig.get_id()));

    auto x = msg.decode_message(key);

    if (msg.get_id() == Packets::SendMessagePacket) {
      msg.set_content(Packets::SendMessagePacket,
                      {x[0], client->uname}, key);
      broadcast(msg);

      spdlog::get("incoming")
          ->info(client->uname + ": '" +
                 x[0] + "'");
    } else
    if (msg.get_id() == Packets::CommandResponsePacket) {
      auto cmd = x;
      simp::message<Packets> response;
      spdlog::info(client->uname + " issued command: " + cmd[0]);

      if (cmd[0] == "/") {
        response.set_content(Packets::CommandResponsePacket, {"simpv2 parse error: This packet cannot be sent without a body"}, key);
      } else if (cmd[0] == "/help") {
        response.set_content(Packets::CommandResponsePacket, {"This version of the protocol does not feature slash commands. Commands '/info' and '/test' are available commands provided by simp2 core "}, key);
      } else if (cmd[0] == "/info") {
        response.set_content(Packets::CommandResponsePacket, {"You are connected to an encrypted openSIMP v2 server, your client supports encryption"}, key);
      } else if (cmd[0] == "/test") {
        response.set_content(Packets::CommandResponsePacket, {"Test succeeded. Your packets are being transferred correctly"}, key);
      } else {
        response.set_content(Packets::CommandResponsePacket, {"Command not found. type /help for a list of commands (simp v3+ only)"}, key);
      }
      
      this->message_client(client, response);
    }
}
