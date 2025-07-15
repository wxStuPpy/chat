#include "MysqlMgr.hpp"
MysqlMgr::~MysqlMgr() {
}
int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return _dao.registerUser(name, email, pwd);
}
MysqlMgr::MysqlMgr() {
}