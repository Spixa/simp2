#pragma once

#include "queue.hpp"
#include "message.hpp"
#include "common.hpp"
#include <asio/connect.hpp>
#include <sys/ucontext.h>
#include <system_error>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/ansicolor_sink.h"

namespace simp {
  using asio::ip::tcp;
  template<typename T>
  class connection : public std::enable_shared_from_this<connection<T>>
  {
  public:
    enum class owner {
      server, client
    };
  public:
    connection(owner parent, asio::io_context& ctx, tcp::socket socket, threadsafe::queue<owned_message<T>>& in)
      : context_(ctx), socket_(std::move(socket)), message_in_queue_(in)
    {
      owner_type_ = parent;
    }

    virtual ~connection()
    {}

    uint32_t get_id() {
      return id_;
    }
  public:
    void connect_to_client(uint32_t uid = 0) {
      if (owner_type_ == owner::server) {
        if (socket_.is_open()) {
          id_ = uid;
          read_header();
        }
      }
    }

    void connect_to_server(const tcp::resolver::results_type& endpoints) {
      if (owner_type_ == owner::client) {
        asio::async_connect(socket_, endpoints,
          [this](std::error_code ec, tcp::endpoint endpoint) {
            if (!ec) {
              read_header();
            }
          }
        );
      }      
    }
    
    void disconnect() {
      if (is_connected()) {
        asio::post(context_, [this](){socket_.close();});
      }
    }
    
    bool is_connected() const {
      return socket_.is_open();
    }

    void listen() {

    }

  public:
    void send(const message<T>& msg) {
      asio::post(context_,
        [this, msg]() 
        {
          bool writing_message = !message_out_queue_.empty();
          message_out_queue_.push_back(msg);

          if (!writing_message) {
            write_header();
          }
        }
      );
    }
  private:
    /* async */ 
    void write_header() {
      asio::async_write(socket_, asio::buffer(&message_out_queue_.front().header, sizeof(message_header)),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            if (message_out_queue_.front().body.size() > 0) {
              write_body();
            } else {
              message_out_queue_.pop_front();

              if (!message_out_queue_.empty()) {
                write_header();
              }
            }
          } else {
            spdlog::critical("Failed header write on " + std::to_string(id_));
            socket_.close();
          }
        }
      );
    }

    /* async */ 
    void write_body() {
      asio::async_write(socket_, asio::buffer(message_out_queue_.front().body.data(), message_out_queue_.front().body.size()),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            message_out_queue_.pop_front();

            if (!message_out_queue_.empty()) {
              write_header();
            }
          } else {
            spdlog::critical("Failed body write on " + std::to_string(id_));
            socket_.close();
          }
        }
      );
    }

private: // reading
  /* async */
  void read_header() {
    asio::async_read(socket_, asio::buffer(&temporary_in_message.header, sizeof(message_header)),
      [this](std::error_code ec, std::size_t length) {
        if (!ec) {
          if (temporary_in_message.header.size > 0) {
            temporary_in_message.body.resize(temporary_in_message.header.size);
            read_body();
          } else {
            add_to_incoming_message_queue();
          }
        } else {
          spdlog::critical("Failed header read on " + std::to_string(id_));
          socket_.close();
        }
      }
    );
  }

  /* async */
  void read_body() {
    asio::async_read(socket_, asio::buffer(temporary_in_message.body.data(), temporary_in_message.body.size()),
      [this](std::error_code ec, std::size_t length) {
        if (!ec) {
          add_to_incoming_message_queue();
        } else {
          spdlog::critical("Failed body read on " + std::to_string(id_));
          socket_.close();
        }
      }
    );
  }

  void add_to_incoming_message_queue() {
    if (owner_type_ == owner::server) {
      message_in_queue_.push_back({ this->shared_from_this(), temporary_in_message});
    } else {
      message_in_queue_.push_back({nullptr, temporary_in_message});
    }

    read_header();
  }

  protected:
    tcp::socket socket_;
    asio::io_context& context_;

    threadsafe::queue<message<T>> message_out_queue_;
    threadsafe::queue<owned_message<T>>& message_in_queue_;

    owner owner_type_ = owner::server;
    uint32_t id_ = 0;

    message<T> temporary_in_message;
  };
};
