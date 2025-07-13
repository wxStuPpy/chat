#include "MysqlDao.hpp"
#include "ConfigMgr.hpp"

MysqlDao::MysqlDao()
{
	auto & cfg = ConfigMgr::getInstance();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	_connectionPool.reset(new MySqlPool(host+":"+port, user, pwd,schema, 5));
}

MysqlDao::~MysqlDao(){
	_connectionPool->close();
}

int MysqlDao::registerUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto conn = _connectionPool->getConnection();
	try {
		if (conn == nullptr) {
			return false;
		}
		// 准备调用存储过程
		unique_ptr < sql::PreparedStatement > stmt(conn->_PConn->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// 设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

		  // 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	   // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
	   unique_ptr<sql::Statement> stmtResult(conn->_PConn->createStatement());
	  unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
	  if (res->next()) {
	       int result = res->getInt("result");
	      std::cout << "Result: " << result << std::endl;
		  _connectionPool->returnConnection(std::move(conn));
		  return result;
	  }
	  _connectionPool->returnConnection(std::move(conn));
		return -1;
	}
	catch (sql::SQLException& e) {
		_connectionPool->returnConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::checkEmail(const std::string& name, const std::string& email) {
	auto conn = _connectionPool->getConnection();
	try {
		if (conn == nullptr) {
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(conn->_PConn->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// 绑定参数
		pstmt->setString(1, name);

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// 遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				_connectionPool->returnConnection(std::move(conn));
				return false;
			}
			_connectionPool->returnConnection(std::move(conn));
			return true;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		_connectionPool->returnConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::updatePwd(const std::string& name, const std::string& newpwd) {
	auto conn = _connectionPool->getConnection();
	try {
		if (conn == nullptr) {
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(conn->_PConn->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// 绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// 执行更新
		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		_connectionPool->returnConnection(std::move(conn));
		return true;
	}
	catch (sql::SQLException& e) {
		_connectionPool->returnConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::checkPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto conn = _connectionPool->getConnection();
	if (conn == nullptr) {
		return false;
	}

	Defer defer([this, &conn]() {
		_connectionPool->returnConnection(std::move(conn));
		});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(conn->_PConn->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); // 将username替换为你要查询的用户名

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// 遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// 输出查询到的密码
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo._name = res->getString("name");
		userInfo._email = email;
		userInfo._uid = res->getInt("uid");
		userInfo._pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::testProcedure(const std::string& email, int& uid, string& name) {
	auto conn = _connectionPool->getConnection();
	try {
		if (conn == nullptr) {
			return false;
		}

		Defer defer([this, &conn]() {
			_connectionPool->returnConnection(std::move(conn));
			});
		// 准备调用存储过程
		unique_ptr < sql::PreparedStatement > stmt(conn->_PConn->prepareStatement("CALL test_procedure(?,@userId,@userName)"));
		// 设置输入参数
		stmt->setString(1, email);
		
		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

		  // 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	   // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		unique_ptr<sql::Statement> stmtResult(conn->_PConn->createStatement());
		unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @userId AS uid"));
		if (!(res->next())) {
			return false;
		}
		
		uid = res->getInt("uid");
		std::cout << "uid: " << uid <<std::endl;
		
		stmtResult.reset(conn->_PConn->createStatement());
		res.reset(stmtResult->executeQuery("SELECT @userName AS name"));
		if (!(res->next())) {
			return false;
		}
		
		name = res->getString("name");
		std::cout << "name: " << name <<std::endl;
		return true;

	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
