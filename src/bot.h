#pragma once

#include <pqxx/connection>
#include <tgbot/Bot.h>

#include "states_controller.h"
#include "config.h"

namespace hanley_bot {

class Bot {
public:
	explicit Bot(config::Config& config);

public:
	void Run();

	//private:
	TgBot::Bot bot_;
	pqxx::connection database_;
	state::StatesController dialogs_;
	config::BotConfig config_;
};



} // namespace hanley_bot