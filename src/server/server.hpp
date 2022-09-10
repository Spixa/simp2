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

#include "asio.hpp"
#include "../common/protocol.hpp"

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
  virtual void onMessage(std::array<char, MAX_IP_PACK_SIZE>& msg) = 0;
};

class ChatRoom {
public:
  void enter(std::shared_ptr<Participant> participant, const std::string& name) {
    participants_.insert(participant);
    name_table_[participant] = name;

    std::array<char, MAX_IP_PACK_SIZE> msg = {"Joined"};
    broadcast(msg, participant);

    std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
      std::bind(&Participant::onMessage, participant, std::placeholders::_1)
    );
  }

  void leave(std::shared_ptr<Participant> participant) {
    std::array<char, MAX_IP_PACK_SIZE> msg = {"Left"};
    broadcast(msg, participant);

    participants_.erase(participant);
    name_table_.erase(participant);
  }

  void broadcast(std::array<char, MAX_IP_PACK_SIZE>& msg, std::shared_ptr<Participant> participant) {
    std::string timestamp = get_timestap();
    std::string name = get_name(participant);
    
    std::array<char, MAX_IP_PACK_SIZE> formatted_msg;

    strcpy(formatted_msg.data(), timestamp.c_str());
    strcat(formatted_msg.data(), name.c_str());
    strcat(formatted_msg.data(), msg.data());

    recent_msgs_.push_back(formatted_msg);
    while (recent_msgs_.size() > max_recent_msgs) {
      recent_msgs_.pop_front();
    }

    std::cout << "(#" << channel_name << ") " << formatted_msg.data() << std::endl;
    
    std::for_each(participants_.begin(), participants_.end(), std::bind(&Participant::onMessage, std::placeholders::_1, std::ref(formatted_msg)));
  }

  std::string get_name(std::shared_ptr<Participant> participant) {
    return name_table_[participant];
  }

private:
  enum { max_recent_msgs = 100 };
  std::unordered_set<std::shared_ptr<Participant>> participants_;
  std::unordered_map<std::shared_ptr<Participant>, std::string> name_table_;
  std::deque<std::array<char, MAX_IP_PACK_SIZE>> recent_msgs_;
  std::string channel_name = "main";
};

class Client : public Participant, public std::enable_shared_from_this<Client> {
public:
  Client(asio::io_service& io_service, asio::io_service::strand& strand, ChatRoom& room) 
    : socket_(io_service), strand_(strand), room_(room)
  {}

  tcp::socket& sock() { return socket_; }

  void start() {
    asio::async_read(socket_, asio::buffer(name_, name_.size()), strand_.wrap(std::bind(&Client::name_handler, shared_from_this(), std::placeholders::_1)));
  }

  void onMessage(std::array<char, simp::MAX_IP_PACK_SIZE>& msg) override {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);

    if (!write_in_progress) {
      asio::async_write(socket_,
        asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
        strand_.wrap(std::bind(&Client::write_handler, shared_from_this(), std::placeholders::_1))
      );
    }
  }

private:
  void name_handler(const std::error_code& error) {
    if (strlen(name_.data()) <= MAX_NICKNAME - 2)
    {
      strcat(name_.data(), ": ");
    } else {
      name_[MAX_NICKNAME - 2] = ':';
      name_[MAX_NICKNAME - 1] = ' ';
    }
    
    room_.enter(shared_from_this(), std::string(name_.data()));

    asio::async_read(socket_, asio::buffer(read_msg_, read_msg_.size()), strand_.wrap(std::bind(&Client::read_handler, shared_from_this(), std::placeholders::_1)));
  }

  void read_handler(std::error_code const& error) {
    if (!error) {
      room_.broadcast(read_msg_, shared_from_this());

      asio::async_read(socket_, asio::buffer(read_msg_, read_msg_.size()), strand_.wrap(std::bind(&Client::read_handler, shared_from_this(), std::placeholders::_1)));
    } else {
      room_.leave(shared_from_this());
    }
  }

  void write_handler(std::error_code const& error) {
    if (!error) {
      write_msgs_.pop_front();

      if (!write_msgs_.empty()) {
        asio::async_write(socket_, asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
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
  std::array<char, simp::MAX_NICKNAME> name_;
  std::array<char, simp::MAX_IP_PACK_SIZE> read_msg_;
  std::deque<std::array<char, simp::MAX_IP_PACK_SIZE>> write_msgs_; 
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