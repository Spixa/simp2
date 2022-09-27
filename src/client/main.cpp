
// #include <curses.h>

// int main(void)
// {
//     bool done = FALSE;
//     WINDOW *input, *output;
//     char buffer[1024];

//     initscr();
//     cbreak();
//     echo();
//     input = newwin(1, COLS, LINES - 1, 0);
//     output = newwin(LINES - 1, COLS, 0, 0);
//     wmove(output, LINES - 2, 0);    /* start at the bottom */
//     scrollok(output, TRUE);
//     while (!done) {
//       mvwprintw(input, 0, 0, "> ");
//       if (wgetnstr(input, buffer, COLS - 4) != OK) {
//           break;
//       }
//       werase(input);
//       waddstr(output, buffer);
//       waddch(output, '\n');   /* result from wgetnstr has no newline */
//       wrefresh(output);
//       done = (*buffer == 4);  /* quit on control-D */
//     }
//     endwin();
//     return 0;
// }

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
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/spdlog.h"

using namespace simp;
using namespace simp::cipher;
class Client : public simp::client_interface<Packets> {
public:
  Client() : client_interface<Packets>() {
    e = rsa::rsa_encryptor::create();
    d = rsa::rsa_decryptor::create();
  }

  void message_all(const std::string &data, const std::string &aes = "") {
    simp::message<Packets> msg;
    msg.set_content(Packets::SendMessagePacket, {data});
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

int main() {

  // std::ostringstream _oss;

  // auto ostream_logger = spdlog::get("console");
  // if (!ostream_logger)
  // {
  //     auto ostream_sink =
  //     std::make_shared<spdlog::sinks::ostream_sink_mt>(_oss); ostream_logger
  //     = std::make_shared<spdlog::logger>("console", ostream_sink);
  //     ostream_logger->set_pattern("%^%L%$ [%H:%M:%S %z] (%n) > %v");
  //     ostream_logger->set_level(spdlog::level::info);
  // }
  // spdlog::set_default_logger(ostream_logger);

  Client c;
  c.connect("127.0.0.1", 37549);

  std::thread send_thread([&c] {
    while (c.is_connected()) {
      std::string buf;
      std::getline(std::cin, buf);

      c.message_all(buf);
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
          auto credits = c.e->encrypt(0, "spixa\x01spixa1937");

          for (auto x : credits) {
            cred.body.push_back(x);
          }

          cred.header.size = cred.body.size();

          c.send(cred);
          std::cout << "Sent credientials\n";
          c.auth_state = Client::AuthState::SentAuth;

          std::stringstream oss;
          oss << c.d.get()->public_key();
          const auto pk = oss.str();
          pubkey.body = std::vector<uint8_t>(pk.begin(), pk.end());
          pubkey.header.size = msg.body.size();

          c.send(pubkey);
          std::cout << "Sent pubkey\n";
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
            std::cout << "Couldn't decode AES key from server\n";
            c.disconnect();
            continue;
          }

          std::cout << "aes key: ";
          for (auto x : aes_key_buf) {
            std::cout << x;
          }
          c.auth_state = Client::AuthState::ReceivedAesKey;

          continue;
        }

        auto x = msg.decode_message();

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
