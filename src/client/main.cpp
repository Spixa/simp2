
#include "../common/message.hpp"
#include "client.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <curses.h>
#include <future>
#include <mutex>
#include <ostream>
#include <ratio>
#include <sstream>
#include <string>
#include <thread>

#include "../common/rsa.hpp"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace simp;
using namespace simp::cipher;

std::string aeskey;

class Client : public simp::client_interface<Packets> {
public:
  Client() : client_interface<Packets>() {
    e = rsa::rsa_encryptor::create();
    d = rsa::rsa_decryptor::create();
  }

  void message_all(const std::string &data) {
    simp::message<Packets> msg;
    msg.set_content(Packets::SendMessagePacket, {data}, aeskey);
    send(msg);
  }

  void send_command(std::string const& command) {
    simp::message<Packets> msg;
    msg.set_content(Packets::CommandResponsePacket, {command}, aeskey);

    send(msg);
  }

  enum class AuthState {
    Unauthenticated,
    GotServerModulo,
    SentAuth,
    ModuloSent,
    ReceivedAesKey,
    Authenticated
  } auth_state = AuthState::Unauthenticated;

  rsa::rsa_encryptor::ptr e;
  rsa::rsa_decryptor::ptr d;
};

std::string aes;

int main(int argc, char** argv) {
  auto console = spdlog::stdout_color_mt("console");

  spdlog::set_default_logger(console);
  spdlog::set_pattern("%^%L%$ > %v");

  std::string uname, passwd, ip, port;
  if (argc > 1) {
    uname = argv[1];
    passwd = "1234";
    ip = "127.0.0.1";
    port = "37549";
  } else {

    std::cout << "Enter username:    ";
    std::getline(std::cin, uname);
    std::cout << "Enter password:    ";
    std::getline(std::cin, passwd);
    std::cout << "Enter server IP:   ";
    std::getline(std::cin, ip);
    std::cout << "Enter server port: ";
    std::getline(std::cin, port);
  }

  Client c;
  c.connect(ip, std::stoi(port));

  std::thread send_thread([&c] {
    while (c.is_connected()) {
      std::string buf;
      std::getline(std::cin, buf);

      if (c.auth_state == Client::AuthState::ReceivedAesKey) {
        if (buf[0] != '/')
          c.message_all(buf);
        else {
          c.send_command(buf);
          spdlog::info("Sending command: " + buf);
        }
      }
    }
  });

  while (true) {

    if (c.is_connected()) {

      if (!c.incoming().empty()) {
        auto msg = c.incoming().pop_front().msg;

        if (c.auth_state == Client::AuthState::Unauthenticated) {
          std::cout << "header size: " << int(msg.header.size) << std::endl;

          BigInt key = BigInt(std::string(
              reinterpret_cast<char *>(msg.body.data()), msg.header.size));

          c.e->new_key(0, key);
          c.auth_state = Client::AuthState::GotServerModulo;

          simp::message<Packets> cred, pubkey;
          auto credits = c.e->encrypt(0, uname + "\x01" + passwd);

          for (auto x : credits) {
            cred.body.push_back(x);
          }

          cred.header.size = cred.body.size();

          c.send(cred);
          spdlog::info("Sent credientials");
          c.auth_state = Client::AuthState::SentAuth;

          std::stringstream oss;
          oss << c.d.get()->public_key();
          const auto pk = oss.str();
          pubkey.body = std::vector<uint8_t>(pk.begin(), pk.end());
          pubkey.header.size = msg.body.size();

          c.send(pubkey);
          spdlog::info("Sent pubkey");
          c.auth_state = Client::AuthState::ModuloSent;
          continue;
        }

        if (c.auth_state == Client::AuthState::ModuloSent) {
          secure_vector<uint8_t> aes_key_buf, ciphertext;

          for (auto x : msg.body) {
            ciphertext.push_back(x);
          }

          try {
            aes_key_buf = c.d.get()->decrypt(ciphertext);
          } catch (Botan::Decoding_Error const &error) {
            spdlog::error("Couldn't decode AES key from server");
            c.disconnect();
            continue;
          }

          for (auto x : aes_key_buf) {
            aeskey += x;
          }
          c.auth_state = Client::AuthState::ReceivedAesKey;

          continue;
        }

        auto x = msg.decode_message(aeskey.data());

        switch (msg.get_id()) {
        case Packets::SendMessagePacket: {
          spdlog::info(x[1] + ": \"" + x[0] + "\"");
        } break;
        case Packets::JoinMessagePacket: {
          spdlog::info(x[0] + " connected");
        } break;
        case Packets::LeaveMessagePacket: {
          spdlog::info(x[0] + " disconnected");
        } break;
        case Packets::CommandResponsePacket: {
          spdlog::info("Command response: " + x[0]);
        }
        default: {

        } break;
        }
      }

    } else {
      spdlog::warn("Server is down");
      break;
    }
  }

  send_thread.join();
}
