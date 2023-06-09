#pragma once

#include <optional>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <tgbot/types/User.h>
#include <tgbot/types/ChatMember.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/Chat.h>
#include <tgbot/Api.h>
#include <tgbot/Bot.h>
#include <tgbot/EventBroadcaster.h>

#undef SendMessage

#include "states_controller.h"
#include "domain.h"
#include "config.h"

namespace hanley_bot {

class Bot {
public:
	explicit Bot(config::Config& config);

	Bot(const Bot&) = delete;
	Bot(Bot&&) = delete;
	Bot& operator=(const Bot&) = delete;
	Bot& operator=(Bot&&) = delete;

public:
	void Run();

	[[nodiscard]] pqxx::work& BeginTransaction();

	void Commit();

	void EndTransaction();

	[[nodiscard]] bool IsOwner(domain::UserID user) const;

	[[nodiscard]] bool IsOwner(const TgBot::User::Ptr& user) const;

	[[nodiscard]] bool IsOwner(const TgBot::ChatMember::Ptr& user) const;

	[[nodiscard]] bool IsOwner(const TgBot::Message::Ptr& message) const;

	[[nodiscard]] bool IsMainGroup(const TgBot::Chat::Ptr& chat) const;

	[[nodiscard]] bool IsFromNewsThread(const TgBot::Message::Ptr& message) const;

	[[nodiscard]] const TgBot::Api& GetAPI();

	[[nodiscard]] domain::UserID GetOwnerID() const;

	void RegisterCommand(const std::string& name, const TgBot::EventBroadcaster::MessageListener& listener);

	TgBot::Message::Ptr SendMessage(domain::ChatID chat_id, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false,
		bool protectContent = false, bool disableNotification = false, domain::ThreadID thread_id = 0);

	TgBot::Message::Ptr SendMessage(const TgBot::Message::Ptr& get_from_message, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false,
		bool protectContent = false, bool disableNotification = false);

	TgBot::Message::Ptr SendMessage(const domain::Context& get_from_context, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false,
		bool protectContent = false, bool disableNotification = false);

	TgBot::Message::Ptr EditMessage(domain::ChatID chat_id, domain::MessageID message_id, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false);

	TgBot::Message::Ptr EditMessage(const TgBot::Message::Ptr& message_to_edit, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false);

	TgBot::Message::Ptr EditMessage(const domain::Context& message_to_edit, const std::string& text,
		TgBot::GenericReply::Ptr replyMarkup = std::make_shared<TgBot::GenericReply>(),
		const std::string& parseMode = "", bool disableWebPagePreview = false);

	bool Typing(domain::ChatID chat_id);

	void AnswerCallbackQuery(const std::string& query_id, const std::string& text = "",
		bool show_alert = false, std::int32_t cache_time = 0);

	double GetRate();

private:
	TgBot::Bot bot_;
	pqxx::connection database_;
	state::StatesController dialogs_;
	config::Config config_;
	std::optional<pqxx::work> current_transaction_;
};



} // namespace hanley_bot