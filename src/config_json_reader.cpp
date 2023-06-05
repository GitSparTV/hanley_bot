#include <fstream>
#include <filesystem>

#include "sdk.h"

#include <boost/json.hpp>

#include "logging.h"
#include "config.h"
#include "config_json_reader.h"

namespace hanley_bot::config {

BotCredentials tag_invoke(const boost::json::value_to_tag<BotCredentials>&, boost::json::value const& json_value) {
	return {
		.bot_token = boost::json::value_to<std::string>(json_value.at("bot_token")),
		.database_uri = boost::json::value_to<std::string>(json_value.at("database_uri")),
	};
}

BotConfig tag_invoke(const boost::json::value_to_tag<BotConfig>&, boost::json::value const& json_value) {
	return {
		.bot_id = json_value.at("bot_id").get_int64(),
		.owner_id = json_value.at("owner_id").get_int64(),
		.group_id = json_value.at("group_id").get_int64(),
		.news_thread_id = static_cast<int>(json_value.at("news_thread_id").get_int64()),
	};
}

Config tag_invoke(const boost::json::value_to_tag<Config>&, boost::json::value const& json_value) {
	return {
		.credentials = boost::json::value_to<BotCredentials>(json_value.at("credentials")),
		.bot_config = boost::json::value_to<BotConfig>(json_value.at("bot_config"))
	};
}

Config FromJSONFile(std::filesystem::path path) {
	std::ifstream file(path, std::ios::binary);

	if (!file) {
		LOG_VERBOSE(fatal) << "Failed to read config file. path=" << path;

		exit(EXIT_FAILURE);
	}

	std::ostringstream file_content_stream;
	file_content_stream << file.rdbuf();

	boost::json::value config_json = boost::json::parse(file_content_stream.str());

	return boost::json::value_to<Config>(config_json);
}

} // namespace hanley_bot::config