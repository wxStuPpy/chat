#include "CServer.hpp"

int main(int argc, char **argv) {
  try {
    unsigned short port = static_cast<unsigned short>(std::atoi(argv[1]));
    net::io_context ioc{1};
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](auto, auto) { ioc.stop(); });
    std::make_shared<CServer>(ioc, port)->start();
    std::cout<<"server start on "<<port<<" port"<<std::endl;
    ioc.run();
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}