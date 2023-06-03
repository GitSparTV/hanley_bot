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

	[[nodiscard]] pqxx::work BeginTransaction();

	[[nodiscard]] bool IsOwner(const TgBot::User::Ptr& user) const;

	[[nodiscard]] bool IsOwner(const TgBot::ChatMember::Ptr& user) const;

	[[nodiscard]] bool IsOwner(const TgBot::Message::Ptr& message) const;

	[[nodiscard]] bool IsMainGroup(const TgBot::Chat::Ptr& chat) const;

	[[nodiscard]] bool IsFromNewsThread(const TgBot::Message::Ptr& message) const;

	[[nodiscard]] const TgBot::Api& GetAPI();

	[[nodiscard]] config::UserID GetOwnerID() const;

	void RegisterCommand(const std::string& name, const TgBot::EventBroadcaster::MessageListener& listener);

	TgBot::Message::Ptr SendMessage(config::ChatID chat_id, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false,
		bool protectContent = false, bool disableNotification = false, config::ThreadID thread_id = 0);

	TgBot::Message::Ptr SendMessage(const TgBot::Message::Ptr& get_from_message, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false,
		bool protectContent = false, bool disableNotification = false);

	bool Typing(config::ChatID chat_id);

private:
	TgBot::Bot bot_;
	pqxx::connection database_;
	state::StatesController dialogs_;
	config::BotConfig config_;
};



} // namespace hanley_bot