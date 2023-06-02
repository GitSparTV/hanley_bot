#pragma once

#include <string>

namespace hanley_bot::config {

struct BotConfig {
    std::int64_t owner_id;
    std::int64_t group_id;
    std::int32_t news_thread_id;
};

struct BotCredentials {
    std::string bot_token;
    std::string database_uri;
};

struct Config {
    BotCredentials credentials;
    BotConfig bot_config;
};

} // namespace hanley_bot::config