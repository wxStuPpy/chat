#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <fstream>

enum class LogLevel {
    trace,
    debug,
    info,
    warning,
    error,
    fatal
};

class Logger {
public:
    static void init(const std::string& logFile = "logs/server_%Y-%m-%d_%H-%M-%S.%N.log");

    static void log(LogLevel level, const std::string& message);
};
