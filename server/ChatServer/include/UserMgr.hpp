#pragma once
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class CSession;
class UserMgr: public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr();
	std::shared_ptr<CSession> getSession(int uid);
	void setUserSession(int uid, std::shared_ptr<CSession> session);
	void rmvUserSession(int uid);
private:
	UserMgr();
	std::mutex _session_mtx;
	std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};

