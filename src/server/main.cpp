#include <asio/io_service.hpp>
#include <thread>
#include "server.hpp"

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: simpserver <port>\n";
    }

    std::shared_ptr<asio::io_service> io_service(new asio::io_service);
    std::shared_ptr<asio::io_service::work> work(new asio::io_service::work(*io_service));
    std::shared_ptr<asio::io_service::strand> strand(new asio::io_service::strand(*io_service));

    std::cout << "[" << std::this_thread::get_id() << "] Server has begun on port " << argv[1] << '\n';
    
    tcp::endpoint endpoint{tcp::v4(), asio::ip::port_type(std::atoi(argv[1]))};
    std::shared_ptr<simp::Server> server = std::make_shared<simp::Server>(*io_service, *strand, endpoint);

    std::thread thread = std::thread{std::bind(&simp::WorkerThread::run, io_service)};

#ifdef __linux__
    // bind cpu affinity for worker thread in linux
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
  
    thread.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << '\n';
  }
}