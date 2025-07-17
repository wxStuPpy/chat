#include "MysqlDao.hpp"
#include "ConfigMgr.hpp"
#include "Logger.hpp"

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
    if (conn == nullptr) {
        Logger::log(LogLevel::error, "MySQL connection is null in registerUser()");
        return -1;
    }

    try {
        Logger::log(LogLevel::info, "Starting user registration: name=" + name + ", email=" + email);

        // 调用存储过程
        unique_ptr<sql::PreparedStatement> stmt(conn->_PConn->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        stmt->execute();
        Logger::log(LogLevel::info, "Stored procedure reg_user executed successfully.");

        // 获取输出参数
        unique_ptr<sql::Statement> stmtResult(conn->_PConn->createStatement());
        unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        
        if (res->next()) {
            int result = res->getInt("result");
            Logger::log(LogLevel::info, "registerUser result: " + std::to_string(result));
            _connectionPool->returnConnection(std::move(conn));
            return result;
        }

        Logger::log(LogLevel::warning, "registerUser: no result returned from stored procedure.");
        _connectionPool->returnConnection(std::move(conn));
        return -1;
    }
    catch (sql::SQLException& e) {
        Logger::log(LogLevel::error, "SQLException in registerUser(): " + std::string(e.what()) +
            " (MySQL error code: " + std::to_string(e.getErrorCode()) +
            ", SQLState: " + e.getSQLState() + ")");
        _connectionPool->returnConnection(std::move(conn));
        return -1;
    }
}



bool MysqlDao::checkEmail(const std::string& name, const std::string& email) {
    auto conn = _connectionPool->getConnection();
    if (conn == nullptr) {
        Logger::log(LogLevel::error, "checkEmail(): Failed to get MySQL connection.");
        return false;
    }

    try {
        Logger::log(LogLevel::info, "checkEmail(): Checking email for user [" + name + "], expected email: " + email);

        // 准备查询语句
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_PConn->prepareStatement("SELECT email FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        Logger::log(LogLevel::info, "checkEmail(): SQL query executed.");

        while (res->next()) {
            std::string foundEmail = res->getString("email");
            Logger::log(LogLevel::info, "checkEmail(): Found email: " + foundEmail);

            _connectionPool->returnConnection(std::move(conn));

            if (email != foundEmail) {
                Logger::log(LogLevel::warning, "checkEmail(): Email mismatch. Input: " + email + ", DB: " + foundEmail);
                return false;
            }

            Logger::log(LogLevel::info, "checkEmail(): Email matches.");
            return true;
        }

        Logger::log(LogLevel::warning, "checkEmail(): No user found with name: " + name);
        _connectionPool->returnConnection(std::move(conn));
        return false;
    }
    catch (sql::SQLException& e) {
        Logger::log(LogLevel::error,
            "checkEmail(): SQLException: " + std::string(e.what()) +
            " (MySQL error code: " + std::to_string(e.getErrorCode()) +
            ", SQLState: " + e.getSQLState() + ")");

        _connectionPool->returnConnection(std::move(conn));
        return false;
    }
}


bool MysqlDao::updatePwd(const std::string& name, const std::string& newpwd) {
    auto conn = _connectionPool->getConnection();
    if (conn == nullptr) {
        Logger::log(LogLevel::error, "updatePwd(): Failed to get MySQL connection.");
        return false;
    }

    try {
        Logger::log(LogLevel::info, "updatePwd(): Updating password for user [" + name + "].");

        // 准备查询语句
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_PConn->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?")
        );

        // 绑定参数（注意顺序）
        pstmt->setString(1, newpwd);
        pstmt->setString(2, name);

        // 执行更新
        int updateCount = pstmt->executeUpdate();
        Logger::log(LogLevel::info, "updatePwd(): Rows updated: " + std::to_string(updateCount));

        _connectionPool->returnConnection(std::move(conn));

        if (updateCount == 0) {
            Logger::log(LogLevel::warning, "updatePwd(): No rows affected. User [" + name + "] may not exist.");
            return false;
        }

        return true;
    }
    catch (sql::SQLException& e) {
        Logger::log(LogLevel::error,
            "updatePwd(): SQLException: " + std::string(e.what()) +
            " (MySQL error code: " + std::to_string(e.getErrorCode()) +
            ", SQLState: " + e.getSQLState() + ")");

        _connectionPool->returnConnection(std::move(conn));
        return false;
    }
}


bool MysqlDao::checkPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
    auto conn = _connectionPool->getConnection();
    if (conn == nullptr) {
        Logger::log(LogLevel::error, "checkPwd(): Failed to get MySQL connection.");
        return false;
    }

    Defer defer([this, &conn]() {
        _connectionPool->returnConnection(std::move(conn));
    });

    try {
        Logger::log(LogLevel::info, "checkPwd(): Verifying password for email [" + email + "].");

        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_PConn->prepareStatement("SELECT * FROM user WHERE email = ?")
        );
        pstmt->setString(1, email);

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        std::string origin_pwd = "";
        if (res->next()) {
            origin_pwd = res->getString("pwd");
            Logger::log(LogLevel::debug, "checkPwd(): Password from DB: " + origin_pwd);
        } else {
            Logger::log(LogLevel::warning, "checkPwd(): No user found for email [" + email + "].");
            return false;
        }

        // 验证密码
        if (pwd != origin_pwd) {
            Logger::log(LogLevel::warning, "checkPwd(): Password mismatch for email [" + email + "].");
            return false;
        }

        // 填充用户信息
        userInfo._name = res->getString("name");
        userInfo._email = email;
        userInfo._uid = res->getInt("uid");
        userInfo._pwd = origin_pwd;

        Logger::log(LogLevel::info, "checkPwd(): User [" + userInfo._name + "] authenticated successfully.");
        return true;
    }
    catch (sql::SQLException& e) {
        Logger::log(LogLevel::error,
            "checkPwd(): SQLException: " + std::string(e.what()) +
            " (MySQL error code: " + std::to_string(e.getErrorCode()) +
            ", SQLState: " + e.getSQLState() + ")");
        return false;
    }
}


bool MysqlDao::testProcedure(const std::string& email, int& uid, std::string& name) {
	auto conn = _connectionPool->getConnection();
	if (conn == nullptr) {
		Logger::log(LogLevel::error, "testProcedure(): Failed to get MySQL connection.");
		return false;
	}

	Defer defer([this, &conn]() {
		_connectionPool->returnConnection(std::move(conn));
	});

	try {
		Logger::log(LogLevel::info, "testProcedure(): Calling stored procedure test_procedure for email: " + email);

		// 准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> stmt(
			conn->_PConn->prepareStatement("CALL test_procedure(?, @userId, @userName)")
		);
		stmt->setString(1, email);
		stmt->execute();
		Logger::log(LogLevel::debug, "testProcedure(): Stored procedure executed.");

		// 获取 @userId
		std::unique_ptr<sql::Statement> stmtResult(conn->_PConn->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @userId AS uid"));

		if (!res->next()) {
			Logger::log(LogLevel::warning, "testProcedure(): No result for @userId.");
			return false;
		}
		uid = res->getInt("uid");
		Logger::log(LogLevel::debug, "testProcedure(): Fetched uid: " + std::to_string(uid));

		// 获取 @userName
		stmtResult.reset(conn->_PConn->createStatement());
		res.reset(stmtResult->executeQuery("SELECT @userName AS name"));

		if (!res->next()) {
			Logger::log(LogLevel::warning, "testProcedure(): No result for @userName.");
			return false;
		}
		name = res->getString("name");
		Logger::log(LogLevel::debug, "testProcedure(): Fetched name: " + name);

		Logger::log(LogLevel::info, "testProcedure(): Successfully fetched uid and name for email: " + email);
		return true;
	}
	catch (sql::SQLException& e) {
		Logger::log(LogLevel::error,
			"testProcedure(): SQLException: " + std::string(e.what()) +
			" (MySQL error code: " + std::to_string(e.getErrorCode()) +
			", SQLState: " + e.getSQLState() + ")");
		return false;
	}
}
