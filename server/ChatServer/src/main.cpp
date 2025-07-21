#include "LogicSystem.hpp"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.hpp"
#include "CServer.hpp"
#include "ConfigMgr.hpp"
#include "Logger.hpp"

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{    Logger::init("/home/ywx/study/Chat/logs/ChatServer.log");
  Logger::log(LogLevel::info, "ChatServer started.");
    try {
        auto &cfg = ConfigMgr::getInstance();
        auto pool = AsioIOServicePool::getInstance();
        boost::asio::io_context  io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool](auto, auto) {
            io_context.stop();
            pool->stop();
            });
        auto port_str = cfg["ChatServer"]["Port"];
        unsigned short port = static_cast<unsigned short>(atoi(port_str.c_str()));
        CServer s(io_context, port);
        io_context.run();
    }
    catch (std::exception& e) {
       Logger::log(LogLevel::error, "exception: "+ std::string(e.what()));
    }

}