#include "sdk.h"


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

	//strm << boost::json::value{
	//	{"timestamp", to_iso_extended_string(*rec[timestamp])},
	//	{"data", *rec[additional_data]},
	//	{"message", *rec[expr::smessage]}
	//};

	auto ts = *rec[timestamp];
	strm << '[' << rec[logging::trivial::severity] << "] [";
	strm << to_iso_extended_string(ts) << "] ";

	if (rec[logging::trivial::severity] == logging::trivial::debug) {
		strm << '[' << rec[file] << ':' << rec[file_line] << "] ";
	}

	strm << rec[expr::smessage];
}

void Init() {
	namespace keywords = boost::log::keywords;

	logging::add_common_attributes();

	logging::add_file_log(
		keywords::file_name = "logs/%Y-%m-%d.log",
		keywords::format = &Format,
		keywords::auto_flush = true,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0)
	);

	logging::add_console_log(
		std::cout,
		keywords::auto_flush = true,
		keywords::format = &Format
	);
}

} // namespace hanley_bot::logger