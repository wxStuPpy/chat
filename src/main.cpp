#include "CServer.hpp"
#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "const.h"
#include <assert.h>

int main(int argc, char **argv) {

  auto &gCfgMgr = ConfigMgr::getInstance();
  Logger::init("/home/ywx/study/Chat/logs/server.log");
  Logger::log(LogLevel::info, "Server started.");
  std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
  unsigned short gate_port = atoi(gate_port_str.c_str());
  try {
    unsigned short port = gate_port;
    net::io_context ioc{1};
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](auto, auto) { ioc.stop(); });
    std::make_shared<CServer>(ioc, port)->start();
    std::cout << "server start on " << port << " port" << std::endl;
    ioc.run();
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}