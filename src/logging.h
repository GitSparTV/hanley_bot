#pragma once

#include "sdk.h"

#include <filesystem>
#include <string_view>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(log_file_name, "File", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_file_line, "LineN", int)

namespace hanley_bot::logger {

void InitConsole();
void InitFile(std::string log_folder);
void ChangeSeverityFilter(std::string_view name);
void HookSignals();

constexpr std::string_view ExtractFileName(std::string_view file) {
	return file.substr(file.find_last_of(std::filesystem::path::preferred_separator) + 1);
}

} // namespace hanley_bot::logger

#define LOG_VERBOSE(severity) BOOST_LOG_TRIVIAL(severity) << boost::log::add_value(log_file_name,hanley_bot::logger::ExtractFileName(__FILE__)) << boost::log::add_value(log_file_line, __LINE__)
#define LOG(severity) BOOST_LOG_TRIVIAL(severity)