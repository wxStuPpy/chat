#include "Logger.hpp"
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/core.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace sinks   = boost::log::sinks;
namespace expr    = boost::log::expressions;
namespace src     = boost::log::sources;
namespace keywords = boost::log::keywords;

using text_sink = sinks::asynchronous_sink<sinks::text_file_backend>;

static src::severity_logger<logging::trivial::severity_level> lg;

void Logger::init(const std::string& logFile) {
    auto sink = boost::make_shared<text_sink>(
        boost::make_shared<sinks::text_file_backend>(
            keywords::file_name = logFile,
            keywords::rotation_size = 10 * 1024 * 1024,  // 10MB
            keywords::auto_flush = true
        )
    );

    sink->set_formatter(
        expr::stream
            << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
            << "] [" << logging::trivial::severity
            << "] " << expr::smessage
    );

    logging::core::get()->add_sink(sink);
    logging::add_common_attributes();
}

void Logger::log(LogLevel level, const std::string& message) {
    switch (level) {
        case LogLevel::trace:   BOOST_LOG_SEV(lg, logging::trivial::trace) << message; break;
        case LogLevel::debug:   BOOST_LOG_SEV(lg, logging::trivial::debug) << message; break;
        case LogLevel::info:    BOOST_LOG_SEV(lg, logging::trivial::info) << message; break;
        case LogLevel::warning: BOOST_LOG_SEV(lg, logging::trivial::warning) << message; break;
        case LogLevel::error:   BOOST_LOG_SEV(lg, logging::trivial::error) << message; break;
        case LogLevel::fatal:   BOOST_LOG_SEV(lg, logging::trivial::fatal) << message; break;
    }
}
