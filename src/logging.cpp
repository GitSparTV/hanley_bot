#include "sdk.h"

#include <csignal>
#include <string_view>

#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time.hpp>

#include "logging.h"

namespace logging = boost::log;
namespace sinks = boost::log::sinks;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

namespace hanley_bot::logger {

void Format(logging::record_view const& rec, logging::formatting_ostream& strm) {
	namespace expr = boost::log::expressions;

	const auto& ts = *rec[timestamp];
	strm << '[' << rec[logging::trivial::severity] << "]\t[";
	strm << to_iso_extended_string(ts) << "]\t";

	if (!rec[log_file_name].empty()) {
		strm << '[' << rec[log_file_name] << ':' << rec[log_file_line] << "]\t";
	}

	strm << rec[expr::smessage];
}
namespace keywords = boost::log::keywords;

void ChangeSeverityFilter(std::string_view name) {
	auto severity = logging::trivial::info;

	if (name == "trace") {
		severity = logging::trivial::info;
	} else if (name == "debug") {
		severity = logging::trivial::debug;
	} else if (name == "info") {
		severity = logging::trivial::info;
	} else if (name == "warning") {
		severity = logging::trivial::warning;
	} else if (name == "error") {
		severity = logging::trivial::error;
	} else if (name == "fatal") {
		severity = logging::trivial::fatal;
	}

	logging::core::get()->set_filter(
		logging::trivial::severity >= severity
	);
}

static const std::unordered_map<int, std::string_view> kSignalsSerialized = {
	{SIGTERM, "SIGTERM"},
	{SIGSEGV, "SIGSEGV"},
	{SIGINT, "SIGINT"},
	{SIGILL, "SIGILL"},
	{SIGABRT, "SIGABRT"},
	{SIGFPE, "SIGFPE"}
};

[[noreturn]] void SignalHandler(int signal) {
	LOG_VERBOSE(fatal) << "Singal "
		<< (kSignalsSerialized.count(signal) ? kSignalsSerialized.at(signal) : std::to_string(signal))
		<< " was caught!";

	exit(signal);
}

void HookSignals() {
	std::signal(SIGTERM, SignalHandler);
	std::signal(SIGSEGV, SignalHandler);
	std::signal(SIGINT, SignalHandler);
	std::signal(SIGILL, SignalHandler);
	std::signal(SIGABRT, SignalHandler);
	std::signal(SIGFPE, SignalHandler);
}

void InitConsole() {
	logging::add_common_attributes();

	logging::add_console_log(
		std::cout,
		keywords::auto_flush = true,
		keywords::format = &Format
	);
}

void InitFile(std::string log_folder) {
	logging::add_common_attributes();

	logging::add_file_log(
		keywords::file_name = std::move(log_folder),
		keywords::open_mode = std::ios::binary | std::ios::app | std::ios::out,
		keywords::format = &Format,
		keywords::auto_flush = true,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0)
	);
}

} // namespace hanley_bot::logger