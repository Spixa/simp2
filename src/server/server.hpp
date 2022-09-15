#pragma once

#include "../common/common.hpp"
#include "../common/connection.hpp"
#include "../common/simp_protocol.hpp"
#include <memory>
#include <thread>

namespace simp {

class server_interface {
public:
  server_interface(uint16_t port) 
    : tcp_acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)), port_(port)
  {

  }

  virtual ~server_interface() {
    stop();
  }

  bool start() {
    try {
      await_clients();
      io_context_thread_ = std::thread([this]() { io_context_.run(); });
    } catch (std::exception& e) {
      std::cerr << "Exception: " << e.what() << std::endl;
      return false;
    }

    std::cout << "Server started.\n\tthread=" << std::this_thread::get_id() << " tcp=v4 port=" << port_ << std::endl;
    return true;
  }

  void stop() {
    io_context_.stop();

    if (io_context_thread_.joinable())
      io_context_thread_.join();
    
    std::cout << "Server stopped normally.\n";
  }

  void await_clients() {
    tcp_acceptor_.async_accept(
      [this](std::error_code ec, tcp::socket socket) {
        if (!ec) {
          std::cout << "[TCP] New connection: " << socket.remote_endpoint() << "\n";

          std::shared_ptr<connection<Packets>> newconn =
            std::make_shared<connection<Packets>>(connection<Packets>::owner::server,
            io_context_, std::move(socket), message_in_queue_);
          
          if (onClientConnect(newconn)) {
            connections_.push_back(std::move(newconn));
            connections_.back()->connect_to_client(id_counter++);

            std::cout << "[SERVER] Connection approved: " << connections_.back()->get_id() << "\n\tTODO: Handshake\n"; 
          } else {
            std::cout << "[SERVER] Previous connection denied\n";
          }
        } else {
          std::cerr << "Error: {new_connection_error}\n";
        }

        await_clients();
      }
    );
  }

  void message_client(std::shared_ptr<connection<Packets>> client, const message<Packets>& msg) {
    if (client && client->is_connected()) {
      client->send(msg);
    } else {
      onClientDisconnect(client);
      client.reset();

      connections_.erase(std::remove(connections_.begin(), connections_.end(), client), connections_.end());
    }
  }

  void broadcast(const message<Packets>& msg, std::shared_ptr<connection<Packets>> ignore_client = nullptr) {
    bool invalid_client_exists = false;

    for (auto& client : connections_) {
      if (client && client->is_connected()) {
        if (client != ignore_client) {
          client->send(msg);
        }
      } else {
        onClientDisconnect(client);
        client.reset();
        invalid_client_exists = true;
      }
    }

    if (invalid_client_exists) {
      connections_.erase(std::remove(connections_.begin(), connections_.end(), nullptr), connections_.end());
    }
  }

  void update(size_t max_messages = -1, bool wait = false)
  {
    if (wait) message_in_queue_.wait();

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

  virtual void onMessage(std::shared_ptr<connection<Packets>>, message<Packets>& msg) {
    
  }
private:
  threadsafe::queue<owned_message<Packets>> message_in_queue_;
  std::deque<std::shared_ptr<connection<Packets>>> connections_;

  asio::io_context io_context_;
  std::thread io_context_thread_;
  uint16_t port_;

  tcp::acceptor tcp_acceptor_;

  uint32_t id_counter = 0;
};

};