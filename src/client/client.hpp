#pragma once

#include "../common/connection.hpp"
#include "../common/simp_protocol.hpp"
#include <asio/io_context.hpp>
#include <exception>

namespace simp {

template<typename T>
class client_interface {
public:
  client_interface()
  {}

  virtual ~client_interface() {
    disconnect();
  }
public:
  bool connect(const std::string& host, const uint16_t port) {
    try {
      tcp::resolver resolver{io_context_};
      tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

      connection_ = std::make_unique<connection<Packets>>(connection<Packets>::owner::client, io_context_, tcp::socket(io_context_), message_in_queue_);

      connection_->connect_to_server(endpoints);
      std::cout << "Succesfully connected to " << host << ":" << int(port) << std::endl;

      io_context_thread_ = std::thread([this](){ io_context_.run(); });
    } catch(std::exception& e) {
      std::cerr << "Client exception: " << e.what() << std::endl;
      return false;
    }
    return true;
  }

  void disconnect() {
    if (is_connected()) {
      connection_->disconnect();
    }

    io_context_.stop();

    if (io_context_thread_.joinable())
      io_context_thread_.join();
    
    connection_.reset();
  }

  bool is_connected() {
    if (connection_)
      return connection_->is_connected();
    return false;
  }

public:
  void send(const message<T>& msg) {
    if (is_connected())
      connection_->send(msg);
  }

  threadsafe::queue<owned_message<Packets>>& incoming() {
    return message_in_queue_;
  }

protected:
  asio::io_context io_context_;
  std::thread io_context_thread_;
  std::unique_ptr<connection<Packets>> connection_;
private:
  threadsafe::queue<owned_message<Packets>> message_in_queue_;
};

};