#pragma once
#include "const.h"

class SqlConnection {
public:
    SqlConnection(sql::Connection* con, int64_t lasttime):_PConn(con), _lastOperTime(lasttime){}
    std::unique_ptr<sql::Connection> _PConn;
    int64_t _lastOperTime;
};

class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : _url(url), _user(user), _pass(pass), _schema(schema), _poolSize(poolSize), _isStop(false){
        try {
            for (int i = 0; i < _poolSize; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                auto*  con = driver->connect(_url, _user, _pass);
                con->setSchema(_schema);
                
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                _connectionPool.push(std::make_unique<SqlConnection>(con, timestamp));
            }

            _checkThread = std::thread([this]() {
                while (!_isStop) {
                    checkConnection();
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
            });

            _checkThread.detach();
        }
        catch (sql::SQLException& e) {
            std::cout << "mysql pool init failed, error is " << e.what()<< std::endl;
        }
    }

    void checkConnection() {
        std::lock_guard<std::mutex> guard(_mutex);
        int poolSize = _connectionPool.size();
        
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        
        for (int i = 0; i < poolSize; i++) {
            auto con = std::move(_connectionPool.front());
            _connectionPool.pop();
            Defer defer([this, &con]() {
                _connectionPool.push(std::move(con));
            });

            if (timestamp - con->_lastOperTime < 5) {
                continue;
            }
            
            try {
                std::unique_ptr<sql::Statement> stmt(con->_PConn->createStatement());
                stmt->executeQuery("SELECT 1");
                con->_lastOperTime = timestamp;
            }
            catch (sql::SQLException& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                auto* newCon = driver->connect(_url, _user, _pass);
                newCon->setSchema(_schema);
                con->_PConn.reset(newCon);
                con->_lastOperTime = timestamp;
            }
        }
    }

    std::unique_ptr<SqlConnection> getConnection() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this] { 
            if (_isStop) {
                return true;
            }        
            return !_connectionPool.empty(); });
            
        if (_isStop) {
            return nullptr;
        }
        
        std::unique_ptr<SqlConnection> con(std::move(_connectionPool.front()));
        _connectionPool.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_isStop) {
            return;
        }
        _connectionPool.push(std::move(con));
        _cond.notify_one();
    }

    void close() {
        _isStop = true;
        _cond.notify_all();
    }

    ~MySqlPool() {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_connectionPool.empty()) {
            _connectionPool.pop();
        }
    }

private:
    std::string _url;
    std::string _user;
    std::string _pass;
    std::string _schema;
    int _poolSize;
    std::queue<std::unique_ptr<SqlConnection>> _connectionPool;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::atomic<bool> _isStop;
    std::thread _checkThread;
};

struct UserInfo {
    std::string _name;
    std::string _pwd;
    int _uid;
    std::string _email;
};

class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    int registerUser(const std::string& name, const std::string& email, const std::string& password);
    bool checkEmail(const std::string& name, const std::string & email);
    bool updatePwd(const std::string& name, const std::string& newPassword);
    bool checkPwd(const std::string& email, const std::string& password, UserInfo& userInfo);
    bool testProcedure(const std::string& email, int& userId, std::string& name);
private:
    std::unique_ptr<MySqlPool> _connectionPool;
};