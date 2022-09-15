#include "../common/message.hpp"
#include "client.hpp"
#include <string>

using namespace simp;
class Client : public simp::client_interface<Packets> {
public:
  void message_all(const std::string& data) {
    simp::message<Packets> msg;
    msg.header.id = Packets::SendMessagePacket; 
    msg.set_content({data});
    send(msg);
  }
};


int main() {
  Client c;
  c.connect("127.0.0.1", 37549);

  std::thread write_thread([&c]()
    {
      while (c.is_connected()) {
        std::string buffer;
        std::getline(std::cin, buffer);

        if (!buffer.empty())
          c.message_all(buffer);
      }
    }
  );

  while (true) {
    if (c.is_connected()) {
      if (!c.incoming().empty()) {
        auto msg = c.incoming().pop_front().msg;

        switch (msg.header.id) {
        case Packets::SendMessagePacket: {
          std::cout <<  msg.body_to_string()[1] << ": '" << msg.body_to_string()[0] << "'\n";
        }
        break;
        case Packets::JoinMessagePacket: {
          std::cout << msg.body_to_string()[0] << " connected\n";
        }
        break;
        case Packets::LeaveMessagePacket: {
          std::cout << msg.body_to_string()[0] << " disconnected\n";
        }
        break;
        }
      }
    } else {
      std::cout << "Server is down\n";
      break;
    }
  }

  write_thread.join();

}