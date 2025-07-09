#pragma once

#include"Singleton.h"
#include<vector>
#include<memory>
#include<boost/asio.hpp>
#include<thread>

class AsioIOServicePool:public Singleton<AsioIOServicePool>
{
    friend class Singleton<AsioIOServicePool>;
public:
    using IOService=boost::asio::io_context;
    using Work=boost::asio::io_context::work;
    using WorkPtr=std::shared_ptr<Work>;

    ~AsioIOServicePool();
    IOService& getIOService();
    AsioIOServicePool& operator=(const AsioIOServicePool&)=delete;
    AsioIOServicePool(const AsioIOServicePool&)=delete;
    void stop();

private:
  AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
  std::vector<IOService> _IOServices;
  std::vector<WorkPtr> _Works;
  std::vector<std::thread> _threads;
  std::size_t _nextIOService;
};
