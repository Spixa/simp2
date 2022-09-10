#include "client.hpp"

#include <cstring>
#include <exception>
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
    array<char, simp::MAX_NICKNAME> nickname;
    strcpy(nickname.data(), argv[1]);

    auto client =  simp::client_interface::create(nickname, io_service, iterator);
    std::thread t(std::bind((unsigned long int (asio::io_service::*)())&asio::io_service::run, &io_service)); 

    std::array<char, simp::MAX_IP_PACK_SIZE> msg;

    while (true)
    {
      memset(msg.data(), '\0', msg.size());
      if (!std::cin.getline(msg.data(), simp::MAX_IP_PACK_SIZE - simp::MAX_NICKNAME))
      {
        std::cin.clear(); //clean up error bit and try to finish reading
      }
      client->write(msg);
    }
  } catch (std::exception& e) {
    std::cout << "e: " << e.what() << std::endl;
  }
}