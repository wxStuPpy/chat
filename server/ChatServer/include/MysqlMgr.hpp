#include "const.h"
#include "MysqlDao.hpp"
class MysqlMgr: public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    int RegUser(const std::string& name, const std::string& email,  const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo&userInfo);
    //测试
    std::shared_ptr<UserInfo> GetUser(int uid){
        std::shared_ptr<UserInfo> user=nullptr;
        return user;
    }
private:
    MysqlMgr();
    MysqlDao  _dao;
};