#pragma once

#include <string>

#include <tgbot/types/User.h>
#include <tgbot/types/Chat.h>
#include <tgbot/types/Message.h>

namespace hanley_bot::config {

using UserID = decltype(TgBot::User::id);
using ChatID = decltype(TgBot::Chat::id);
using ThreadID = decltype(TgBot::Message::messageThreadId);

struct BotCredentials {
	std::string bot_token;
	std::string database_uri;
};

struct BotConfig {
	UserID bot_id;
	UserID owner_id;
	ChatID group_id;
	ThreadID news_thread_id;
};

struct Config {
	BotCredentials credentials;
	BotConfig bot_config;
};

} // namespace hanley_bot::config