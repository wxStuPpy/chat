#pragma once

#include "CSession.hpp"
#include "const.h"

typedef  function<void(shared_ptr<CSession>, const short &msg_id, const string &msg_data)> FunCallBack;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void postMsgToQue(shared_ptr < LogicNode> msg);
private:
	LogicSystem();
	void dealMsg();
	void registerCallBacks();
	void loginHandler(shared_ptr<CSession>, const short &msg_id, const string &msg_data);
	bool getBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	std::thread _worker_thread;
	std::queue<shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;
};

