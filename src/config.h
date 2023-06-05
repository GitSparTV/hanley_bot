#pragma once

#include <string>

#include "domain.h"

namespace hanley_bot::config {

struct BotCredentials {
	std::string bot_token;
	std::string database_uri;
};

struct BotConfig {
	domain::UserID bot_id;
	domain::UserID owner_id;
	domain::ChatID group_id;
	domain::ThreadID news_thread_id;
};

struct Config {
	BotCredentials credentials;
	BotConfig bot_config;
	std::string log_folder;
};

} // namespace hanley_bot::config