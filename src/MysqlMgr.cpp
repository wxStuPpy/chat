#include "MysqlMgr.hpp"
MysqlMgr::~MysqlMgr() {
}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return _dao.registerUser(name, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
    return _dao.checkEmail(name, email);
}
bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
    return _dao.updatePwd(name, pwd);
}

MysqlMgr::MysqlMgr() {
}