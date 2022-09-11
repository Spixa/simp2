#include "client.hpp"

#include <cstring>
#include <exception>
#include <string>
#include <thread>
#include <functional>
#include "asio.hpp"

int main(int argc, char** argv) {
  using std::array;
  using asio::ip::tcp;

  try {
    if (argc != 4) {
      std::cerr << "Usage: simpclient <nickname> <host> <port>\n";
    }

    // io service
    asio::io_service io_service;
    tcp::resolver resolver(io_service);
    
    // query ip and port
    tcp::resolver::query query(argv[2], argv[3]);

    // retrieve endpoints
    tcp::resolver::iterator iterator = resolver.resolve(query);

    // copy nickname from argv[1] to buffer
    std::string name = argv[1];


    auto client =  simp::client_interface::create(name, io_service, iterator);
    std::thread t(std::bind((unsigned long int (asio::io_service::*)())&asio::io_service::run, &io_service)); 

    while (true)
    {
      std::string buffer;
      std::getline(std::cin, buffer);

      simp::chat_message message;
      message.set_message(1, {buffer});
      client->write(message);
    }
  } catch (std::exception& e) {
    std::cout << "e: " << e.what() << std::endl;
  }
}