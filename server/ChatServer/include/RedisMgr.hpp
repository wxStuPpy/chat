#pragma once
#include "const.h"

class RedisConPool;

class RedisMgr: public Singleton<RedisMgr>, 
    public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
  ~RedisMgr();
  bool Get(const std::string &key, std::string &value);
  bool Set(const std::string &key, const std::string &value);
  bool LPush(const std::string &key, const std::string &value);
  bool LPop(const std::string &key, std::string &value);
  bool RPush(const std::string &key, const std::string &value);
  bool RPop(const std::string &key, std::string &value);
  bool HSet(const std::string &key, const std::string &hkey,
            const std::string &value);
  bool HSet(const char *key, const char *hkey, const char *hvalue,
            size_t hvaluelen);
  std::string HGet(const std::string &key, const std::string &hkey);
  bool Del(const std::string &key);
  bool ExistsKey(const std::string &key);
  void Close();
private:
  RedisMgr();
  std::unique_ptr<RedisConPool> _pool;
};

class RedisConPool {
public:
    RedisConPool(size_t _poolSize, const char* _host, int _port, const char* _pwd)
        : _poolSize(_poolSize), _host(_host), _port(_port), _isStop(false) {
        for (size_t i = 0; i < _poolSize; ++i) {
            auto* context = redisConnect(_host, _port);
            if (context == nullptr || context->err != 0) {
                if (context != nullptr) {
                    redisFree(context);
                }
                continue;
            }
            auto reply = (redisReply*)redisCommand(context, "AUTH %s", _pwd);
            if (reply->type == REDIS_REPLY_ERROR) {
                std::cout << "认证失败" << std::endl;
                // 执行成功 释放redisCommand执行后返回的redisReply所占用的内存
                freeReplyObject(reply);
                continue;
            }
            // 执行成功 释放redisCommand执行后返回的redisReply所占用的内存
            freeReplyObject(reply);
            std::cout << "认证成功" << std::endl;
            _connections.push(context);
        }
    }

    ~RedisConPool() {
        std::lock_guard<std::mutex> lock(_mutex);
        while (!_connections.empty()) {
            // 补充：释放Redis连接资源（原代码漏了这步）
            redisFree(_connections.front());
            _connections.pop();
        }
    }

    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this] { 
            if (_isStop) {
                return true;
            }
            return !_connections.empty(); 
        });
        // 如果停止则直接返回空指针
        if (_isStop) {
            return nullptr;
        }
        auto* context = _connections.front();
        _connections.pop();
        return context;
    }

    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_isStop) {
            // 补充：如果池已停止，直接释放归还的连接
            redisFree(context);
            return;
        }
        _connections.push(context);
        _cond.notify_one();
    }

    void close() {
        _isStop = true;
        _cond.notify_all();
    }

private:    
    size_t _poolSize;               
    const char* _host;
    int _port;
    std::atomic<bool> _isStop;  
    std::queue<redisContext*> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
};