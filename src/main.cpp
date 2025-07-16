#include "CServer.hpp"
#include "const.h"
#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include <assert.h>

void TestRedisMgr() {

    assert(RedisMgr::getInstance()->Set("blogwebsite","llfc.club"));
    std::string value="";
    assert(RedisMgr::getInstance()->Get("blogwebsite", value) );
    assert(RedisMgr::getInstance()->Get("nonekey", value) == false);
    assert(RedisMgr::getInstance()->HSet("bloginfo","blogwebsite", "llfc.club"));
    assert(RedisMgr::getInstance()->HGet("bloginfo","blogwebsite") != "");
    assert(RedisMgr::getInstance()->ExistsKey("bloginfo"));
    assert(RedisMgr::getInstance()->Del("bloginfo"));
    assert(RedisMgr::getInstance()->Del("bloginfo"));
    assert(RedisMgr::getInstance()->ExistsKey("bloginfo") == false);
    assert(RedisMgr::getInstance()->LPush("lpushkey1", "lpushvalue1"));
    assert(RedisMgr::getInstance()->LPush("lpushkey1", "lpushvalue2"));
    assert(RedisMgr::getInstance()->LPush("lpushkey1", "lpushvalue3"));
    assert(RedisMgr::getInstance()->RPop("lpushkey1", value));
    assert(RedisMgr::getInstance()->RPop("lpushkey1", value));
    assert(RedisMgr::getInstance()->LPop("lpushkey1", value));
    assert(RedisMgr::getInstance()->LPop("lpushkey2", value)==false);
    RedisMgr::getInstance()->Close();
}

int main(int argc, char **argv) {
  //TestRedisMgr();
  auto &gCfgMgr=ConfigMgr::getInstance();
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
    std::cout<<"server start on "<<port<<" port"<<std::endl;
    ioc.run();
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}