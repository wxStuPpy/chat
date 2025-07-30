#include "LogicSystem.hpp"
#include "StatusGrpcClient.hpp"
#include "MysqlMgr.hpp"
#include "const.h"
#include "RedisMgr.hpp"
#include "UserMgr.hpp"
using namespace std;

LogicSystem::LogicSystem():_b_stop(false){
	registerCallBacks();
	_worker_thread = std::thread (&LogicSystem::dealMsg, this);
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

void LogicSystem::postMsgToQue(shared_ptr < LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);
	//可以增加达到队列上限的逻辑
	
	//由0变为1则发送通知信号
	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}

void LogicSystem::dealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		//判断队列为空则用条件变量阻塞等待，并释放锁
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		//判断是否为关闭状态，把所有逻辑执行完后则退出循环
		if (_b_stop ) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end()) {
					Logger::log(LogLevel::info, 
						"msg id [" + std::to_string(msg_node->_recvnode->_msg_id)+"] handler not found");
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}

		//如果没有停服，且说明队列中有数据
		auto msg_node = _msg_que.front();
		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			_msg_que.pop();
		Logger::log(LogLevel::warning, 
			"msg id [" + std::to_string(msg_node->_recvnode->_msg_id)+"] handler not found");
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id, 
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

void LogicSystem::registerCallBacks() {
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::loginHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
}

bool LogicSystem::getBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::getInstance()->Get(base_key, info_str);
	if (b_base) {
		json root = json::parse(info_str);
		userinfo->uid = root["uid"].get<int>();
		userinfo->name = root["name"].get<std::string>();
		userinfo->pwd = root["pwd"].get<std::string>();
		userinfo->email = root["email"].get<std::string>();
		userinfo->nick = root["nick"].get<std::string>();
		userinfo->desc = root["desc"].get<std::string>();
		userinfo->sex = root["sex"].get<int>();
		userinfo->icon = root["icon"].get<std::string>();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
	}
	else {
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::getInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		//将数据库内容写入redis缓存
		json redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::getInstance()->Set(base_key, redis_root.dump());
	}

	return true;
}


void LogicSystem::loginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Logger::log(LogLevel::info,"loginHandler is called");
	json reader=json::parse(msg_data);
	auto uid = reader["uid"].get<int>();
	auto token = reader["token"].get<std::string>();
	Logger::log(LogLevel::info,"user login uid is  " + std::to_string(uid) + " user token  is "+token);

	//从状态服务器获取token匹配是否准确
	json rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.dump();
		session->send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	//从redis获取用户token是否正确
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::getInstance()->Get(token_key, token_value);
	if (!success) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return ;
	}

	if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return ;
	}

	rtvalue["error"] = ErrorCodes::Success;

	//从状态服务器获取用户信息
	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = getBaseInfo(base_key, uid, user_info);
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

	//从数据库获取申请列表

	//获取好友列表

	auto server_name = ConfigMgr::getInstance().getValue("SelfServer", "Name");
	auto re_res=RedisMgr::getInstance()->HGet(LOGIN_COUNT,server_name);
	int count=0;
	if(!re_res.empty())
	{
		count=std::stoi(re_res);
	}
	count++;

	auto count_str=std::to_string(count);
	RedisMgr::getInstance()->HSet(LOGIN_COUNT,server_name,count_str);
	

	session->setUserID(uid);

	std::string  ipkey = USERIPPREFIX + uid_str;
	RedisMgr::getInstance()->Set(ipkey, server_name);

	UserMgr::getInstance()->setUserSession(uid, session);

	return;
}
