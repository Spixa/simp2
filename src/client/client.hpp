#pragma once

#include <asio.hpp>
#include <asio/io_service.hpp>
#include <asio/write.hpp>
#include <cstring>
#include <system_error>
#include <thread>
#include <string>
#include <utility>
#include <array>
#include <deque>
#include <memory>
#include <iostream>
#include <functional>

#include "../common/protocol.hpp"

using asio::ip::tcp;
using namespace std::placeholders;

namespace simp {

class client_interface {
public:
  static std::shared_ptr<client_interface> create(std::array<char, MAX_NICKNAME>& nickname, asio::io_service& io_service, tcp::resolver::iterator endpoint_iter) {
    return std::make_shared<client_interface>
      (nickname, io_service, endpoint_iter);
  }

  client_interface(std::array<char, MAX_NICKNAME>& nickname, asio::io_service& io_service, tcp::resolver::iterator endpoint_iter)
    : io_service_(io_service), socket_(io_service)
  {
    strcpy(nickname_.data(), nickname.data());
    memset(read_msg_.data(), '\0', MAX_IP_PACK_SIZE);

    asio::async_connect(socket_, endpoint_iter, std::bind(&client_interface::onConnect, this, _1));
  }

  void write(const std::array<char, MAX_IP_PACK_SIZE>& msg)
  {
    io_service_.post(std::bind(&client_interface::write_impl, this, msg));
  }

private:
  void onConnect(std::error_code const& error)
  {
    if (!error) {
      asio::async_write(
        socket_, asio::buffer(nickname_, nickname_.size()),
        std::bind(&client_interface::read_handler, this, _1)
      );
    }
  }

  void read_handler(const std::error_code& error)
  {
    std::cout << read_msg_.data() << std::endl;

    if (!error) {
      asio::async_read(socket_,
        asio::buffer(read_msg_, read_msg_.size()),
        std::bind(&client_interface::read_handler, this, _1)
      );
    } else {
      close_impl();
    }
  }

  void write_impl(std::array<char, MAX_IP_PACK_SIZE> msg)
  {
    bool write_in_progress = !write_msgs_.empty();

    write_msgs_.push_back(msg);

    if (!write_in_progress) {
      asio::async_write(
        socket_, asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
        std::bind(&client_interface::write_handler, this, _1)
      );
    }
  }

  void write_handler(const std::error_code& error)
  {
    if (!error) {
      write_msgs_.pop_front();
      if (!write_msgs_.empty()) {
        asio::async_write(
          socket_, asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
          std::bind(&client_interface::write_handler, this, _1)
        );
      }
    }
  }
  
  void close_impl()
  {
    socket_.close();
  }

private:
  asio::io_service& io_service_;
  tcp::socket socket_;
  
  std::array<char, MAX_IP_PACK_SIZE> read_msg_;
  std::deque<std::array<char, MAX_IP_PACK_SIZE>> write_msgs_;
  std::array<char, MAX_NICKNAME> nickname_;

};

}; // namespace simp