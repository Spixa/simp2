#pragma once

#include <asio/io_service.hpp>
#include <asio/write.hpp>
#include <cstring>
#include <ctime>
#include <string>
#include <deque>
#include <iostream>
#include <list>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>
#include <iomanip>
#include <array>

#include "../common/message.hpp"
#include "asio.hpp"
#include "../common/protocol.hpp"

static int num = 0;

#define DEBUG std::cout << "DEBUG " << num << std::endl; num++;

using asio::ip::tcp;

namespace simp {
namespace {
  std::string get_timestap()
  {
      time_t t = time(0);   // get time now
      struct tm * now = localtime(&t);
      std::stringstream ss;
      ss << '[' << (now->tm_year + 1900) << '-' << std::setfill('0')
        << std::setw(2) << (now->tm_mon + 1) << '-' << std::setfill('0')
        << std::setw(2) << now->tm_mday << ' ' << std::setfill('0')
        << std::setw(2) << now->tm_hour << ":" << std::setfill('0')
        << std::setw(2) << now->tm_min << ":" << std::setfill('0')
        << std::setw(2) << now->tm_sec << "] ";

      return ss.str();
  } 

  class WorkerThread {
  public:
    static void run(std::shared_ptr<asio::io_service> io_service) {
      {
        std::lock_guard<std::mutex> lock(m);
        std::cout << "[" << std::this_thread::get_id() << " Thread begin] Ok" << std::endl;
      }

      io_service->run();

      {
        std::lock_guard<std::mutex> lock(m);
        std::cout << "[" << std::this_thread::get_id() << " Thread end] Ok" << std::endl;
      }
    }
  private:
    static std::mutex m;
  };

  std::mutex WorkerThread::m;
}

class Participant {
public:
  virtual ~Participant() {};
  virtual void onMessage(simp::chat_message& msg) = 0;
};

class ChatRoom {
public:
  void enter(std::shared_ptr<Participant> participant, const std::string& name) {
    participants_.insert(participant);
    name_table_[participant] = name;

    std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
      std::bind(&Participant::onMessage, participant, std::placeholders::_1)
    );
  }

  void leave(std::shared_ptr<Participant> participant) {

    participants_.erase(participant);
    name_table_.erase(participant);
  }

  void broadcast(chat_message& message, std::shared_ptr<Participant> participant) {
    std::string name = get_name(participant);

    auto g = chat_message::decode(message.get_data());

    std::cout << "the raw message we got was this: " << message.get_data().data() << " with this size: " << message.get_data().size() << std::endl;

    chat_message formatted_msg{};

    formatted_msg.set_message(std::get<uint16_t>(g), {std::get<std::vector<std::string>>(g)[0], name});
    recent_msgs_.push_back(formatted_msg);

    while (recent_msgs_.size() > max_recent_msgs) {
      recent_msgs_.pop_front();
    }

    std::cout << "(#" << channel_name << ") " << formatted_msg.get_data().data() << std::endl;

    std::for_each(participants_.begin(), participants_.end(), std::bind(&Participant::onMessage, std::placeholders::_1, (formatted_msg)));
  }

  std::string get_name(std::shared_ptr<Participant> participant) {
    return name_table_[participant];
  }

private:
  enum { max_recent_msgs = 100 };
  std::unordered_set<std::shared_ptr<Participant>> participants_;
  std::unordered_map<std::shared_ptr<Participant>, std::string> name_table_;
  std::deque<chat_message> recent_msgs_;
  std::string channel_name = "main";
};

class Client : public Participant, public std::enable_shared_from_this<Client> {
public:
  Client(asio::io_service& io_service, asio::io_service::strand& strand, ChatRoom& room) 
    : socket_(io_service), strand_(strand), room_(room)
  {}

  tcp::socket& sock() { return socket_; }

  void start() {
    asio::async_read(socket_, asio::buffer(name_.get_data(), name_.get_data().size()), strand_.wrap(std::bind(&Client::name_handler, shared_from_this(), std::placeholders::_1)));
  }

  void onMessage(simp::chat_message& msg) override {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);

    if (!write_in_progress) {
      asio::async_write(socket_,
        asio::buffer(write_msgs_.front().get_data(), write_msgs_.front().get_data().size()),
        strand_.wrap(std::bind(&Client::write_handler, shared_from_this(), std::placeholders::_1))
      );
    }
  }

private:
  void name_handler(const std::error_code& error) {
    if (std::get<uint16_t>(chat_message::decode(name_.get_data())) == 3)
    {
      std::cout << std::string(name_.get_data().data()) << std::endl;
      std::string name = std::get<std::vector<std::string>>(chat_message::decode(name_.get_data()))[0];

      room_.enter(shared_from_this(), name);

      std::cout << name << " connected.\n";
      asio::async_read(socket_, asio::buffer(read_msg_.get_data(), read_msg_.get_data().size()), strand_.wrap(std::bind(&Client::read_handler, shared_from_this(), std::placeholders::_1)));
    } else {
      socket_.close();
    }
  }

  void read_handler(std::error_code const& error) {
    if (!error) {
      auto g = chat_message::decode(read_msg_.get_data());

      if (std::get<uint16_t>(g) == 1) 
        room_.broadcast(read_msg_, shared_from_this());

      asio::async_read(socket_, asio::buffer(read_msg_.get_data(), read_msg_.get_data().size()), strand_.wrap(std::bind(&Client::read_handler, shared_from_this(), std::placeholders::_1)));
    } else {
      room_.leave(shared_from_this());
    }
  }

  void write_handler(std::error_code const& error) {
    if (!error) {
      write_msgs_.pop_front();

      if (!write_msgs_.empty()) {
        asio::async_write(socket_, asio::buffer(write_msgs_.front().get_data(), write_msgs_.front().get_data().size()),
          strand_.wrap(std::bind(&Client::write_handler, this, std::placeholders::_1))
        );
      }
    } else {
      room_.leave(shared_from_this());
    }
  }

private:
  tcp::socket socket_;
  asio::io_service::strand& strand_;
  ChatRoom& room_;
  chat_message name_;
  chat_message read_msg_;
  std::deque<simp::chat_message> write_msgs_; 
};

class Server {
public:
  Server(asio::io_service& io_service, asio::io_service::strand& strand, const tcp::endpoint& endpoint) 
    : io_service_(io_service), strand_(strand), acceptor_(io_service, endpoint)
  {
    run();
  }
protected:
  void run() {
    std::shared_ptr<Client> new_client(new Client(io_service_, strand_, room_));
    acceptor_.async_accept(new_client->sock(), strand_.wrap(std::bind(&Server::onAccept, this, new_client, std::placeholders::_1)));
  }

  void onAccept(std::shared_ptr<Client> client, std::error_code const& error) {
    if (!error) {
      client->start();
    }

    run();
  }
private:
  asio::io_service& io_service_;
  asio::io_service::strand& strand_;
  
  tcp::acceptor acceptor_;
  ChatRoom room_;
};

}