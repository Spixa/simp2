#pragma once

#include "../common/common.hpp"
#include "../common/simp_protocol.hpp"
#include "server_connection.hpp"

// #include <spdlog/spdlog.hpp>
#include <memory>
#include <thread>
#include <string>

namespace simp {

class server_interface {
public:
  server_interface(uint16_t port)
      : tcp_acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
        port_(port) {
    fail_callback_ = std::function<void(std::shared_ptr<connection<Packets>>)>{
        [this](std::shared_ptr<connection<Packets>> c) {
          onClientDisconnect(c);
        }};
  }

  virtual ~server_interface() { stop(); }

  bool start() {
    encryptor = cipher::rsa::rsa_encryptor::create();
    decryptor = cipher::rsa::rsa_decryptor::create();

    try {
      await_clients();
      io_context_thread_ = std::thread([this]() { io_context_.run(); });
    } catch (std::exception &e) {
      spdlog::get("console")->info("Exception: " + std::string(e.what()));
      return false;
    }

    spdlog::info("Simp Server has begun on port " + std::to_string(port_));
    
    std::stringstream thrId;
    thrId << std::this_thread::get_id();

    spdlog::debug("thread: {}", thrId.str());
    return true;
  }

  void stop() {
    io_context_.stop();

    if (io_context_thread_.joinable())
      io_context_thread_.join();

    std::cout << "Server stopped normally.\n";
  }

  void await_clients() {
    tcp_acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::stringstream sock;
        sock << socket.remote_endpoint();
        spdlog::get("auth")->info("Connection: " + sock.str());

        std::shared_ptr<connection<Packets>> newconn =
            std::make_shared<connection<Packets>>(
                connection<Packets>::owner::server, io_context_,
                std::move(socket), message_in_queue_);

        connections_.push_back(std::move(newconn));
        connections_.back()->set_fail_callback(fail_callback_);
        connections_.back()->connect_to_client(id_counter++);

        if (onClientConnect(connections_.back())) {
          spdlog::get("auth")->info("Connection approved");
        } else {
          spdlog::get("auth")->info("Connection declined");
          connections_.pop_back();
        }
      } else {
        spdlog::get("auth")->error("New connection error");
      }

      await_clients();
    });
  }

  void message_client(std::shared_ptr<connection<Packets>> client,
                      const message<Packets> &msg) {
    if (client && client->is_connected()) {
      client->send(msg);
    } else {
      client.reset();

      connections_.erase(
          std::remove(connections_.begin(), connections_.end(), client),
          connections_.end());
    }
  }

  void broadcast(const message<Packets> &msg,
                 std::shared_ptr<connection<Packets>> ignore_client = nullptr) {
    bool invalid_client_exists = false;

    for (auto &client : connections_) {
      if (client && client->is_connected()) {
        if (client != ignore_client &&
            client->auth == connection<Packets>::AuthState::AesKeySent) {
          client->send(msg);
        }
      } else {
        client.reset();
        invalid_client_exists = true;
      }
    }

    if (invalid_client_exists) {
      connections_.erase(
          std::remove(connections_.begin(), connections_.end(), nullptr),
          connections_.end());
    }
  }

  void update(size_t max_messages = -1, bool wait = false) {
    if (wait)
      message_in_queue_.wait();

    size_t message_count = 0;
    while (message_count < max_messages && !message_in_queue_.empty()) {
      auto msg = message_in_queue_.pop_front();
      onMessage(msg.remote, msg.msg);
      message_count++;
    }
  }

protected:
  virtual bool onClientConnect(std::shared_ptr<connection<Packets>> client) {
    return true;
  }

  virtual void onClientDisconnect(std::shared_ptr<connection<Packets>> client) {

  }

  virtual void onMessage(std::shared_ptr<connection<Packets>>,
                         message<Packets> &msg) {}

private:
  threadsafe::queue<owned_message<Packets>> message_in_queue_;
  std::deque<std::shared_ptr<connection<Packets>>> connections_;

  asio::io_context io_context_;
  std::thread io_context_thread_;
  uint16_t port_;

  tcp::acceptor tcp_acceptor_;
  std::function<void(std::shared_ptr<connection<Packets>>)> fail_callback_;
  uint32_t id_counter = 0;

protected:
  cipher::rsa::rsa_encryptor::ptr encryptor;
  cipher::rsa::rsa_decryptor::ptr decryptor;
};

}; // namespace simp
