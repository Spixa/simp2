
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
#include <chrono>
#include <cstring>
#include <curses.h>
#include <ostream>
#include <ratio>
#include <sstream>
#include <mutex>
#include <cstdio>
#include <future>
#include <string>
#include <atomic>
#include <thread>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/ostream_sink.h"

using namespace simp;
class Client : public simp::client_interface<Packets> {
public:
  void message_all(const std::string& data) {
    simp::message<Packets> msg;
    msg.set_content(Packets::SendMessagePacket, {data});
    send(msg);
  }
};


int main() {


  // std::ostringstream _oss;

  // auto ostream_logger = spdlog::get("console");
  // if (!ostream_logger)
  // {
  //     auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(_oss);
  //     ostream_logger = std::make_shared<spdlog::logger>("console", ostream_sink);
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
        auto x = msg.decode_message();

        switch (msg.get_id()) {
        case Packets::SendMessagePacket: {
           spdlog::info(x[1] + ": \"" + x[0] + "\"");
        }
        break;
        case Packets::JoinMessagePacket: {
          spdlog::info(x[0] + " connected");
        }
        break;
        case Packets::LeaveMessagePacket: {
          spdlog::info(x[0] + " disconnected");
        }
        break;
        default: {

        }
        break;
        }
      }

    } else {
      spdlog::warn("Server is down");
      break;
    }
  }
  
  send_thread.join();
}