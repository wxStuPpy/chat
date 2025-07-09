#include"AsioIOServicePool.hpp"

AsioIOServicePool::AsioIOServicePool(std::size_t size )
:_IOServices(size),_Works(size),_nextIOService(0)
{
    for(std::size_t i=0;i<size;++i)
    {
        _Works[i]=std::unique_ptr<Work>(new Work(_IOServices[i]));
        _threads.emplace_back([this,i](){ _IOServices[i].run(); });
    }
}

AsioIOServicePool::~AsioIOServicePool(){
    stop();
    std::cout<<"AsioIOServicePool destructed"<<std::endl;
}

AsioIOServicePool::IOService &AsioIOServicePool::getIOService() {
    auto &IOService = _IOServices[_nextIOService++];
    if (_nextIOService >= _IOServices.size()) {
     _nextIOService = 0;
    }
    return IOService;
}

void AsioIOServicePool::stop(){
    for(auto work: _Works){
        work->get_io_context().stop();
        work.reset();
    }
    for(auto &thread: _threads){
        if(thread.joinable())
          thread.join();
    }
}