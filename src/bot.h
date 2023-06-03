#pragma once

#include <pqxx/connection>
#include <pqxx/transaction>
#include <tgbot/types/User.h>
#include <tgbot/types/ChatMember.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/Chat.h>
#include <tgbot/Api.h>
#include <tgbot/Bot.h>
#include <tgbot/EventBroadcaster.h>

#include "states_controller.h"
#include "config.h"

namespace hanley_bot {

class Bot {
public:
	explicit Bot(config::Config& config);

public:
	void Run();

	pqxx::work MakeTransaction();

	bool IsOwner(const TgBot::User::Ptr& user) const;

	bool IsOwner(const TgBot::ChatMember::Ptr& user) const;

	bool IsOwner(const TgBot::Message::Ptr& message) const;

	bool IsMainGroup(const TgBot::Chat::Ptr& chat) const;

	bool IsFromNewsThread(const TgBot::Message::Ptr& message) const;

	const TgBot::Api& GetAPI();

	config::UserID GetOwnerID() const;

	void RegisterCommand(const std::string& name, const TgBot::EventBroadcaster::MessageListener& listener);

	//private:
	TgBot::Bot bot_;
	pqxx::connection database_;
	state::StatesController dialogs_;
	config::BotConfig config_;
};



} // namespace hanley_bot