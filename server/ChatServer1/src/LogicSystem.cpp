#include "LogicSystem.hpp"
#include "StatusGrpcClient.hpp"
#include "MysqlMgr.hpp"
#include "const.h"

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

void LogicSystem::loginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Logger::log(LogLevel::info,"loginHandler is called");
	json reader=json::parse(msg_data);
	auto uid = reader["uid"].get<int>();
	Logger::log(LogLevel::info,"user login uid is  " + std::to_string(uid) + " user token  is "+reader["token"].get<std::string>());
	//从状态服务器获取token匹配是否准确
	auto rsp = StatusGrpcClient::getInstance()->Login(uid, reader["token"].get<std::string>());
	json rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.dump();
		session->send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	rtvalue["error"] = rsp.error();
	if (rsp.error() != ErrorCodes::Success) {
		return;
	}

	//内存中查询用户信息
	auto find_iter = _users.find(uid);
	std::shared_ptr<UserInfo> user_info = nullptr;
	if (find_iter == _users.end()) {
		//查询数据库
		user_info = MysqlMgr::getInstance()->GetUser(uid);
		if (user_info == nullptr) {
			rtvalue["error"] = ErrorCodes::UidInvalid;
			return;
		}

		_users[uid] = user_info;
	}
	else {
		user_info = find_iter->second;
	}

	rtvalue["uid"] = uid;
	rtvalue["token"] = rsp.token();
	rtvalue["name"] = user_info->name;
}
