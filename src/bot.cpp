#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <chrono>
#include <thread>

#include <tgbot/tgbot.h>
#include <pqxx/pqxx>

#include "bot.h"
#include "domain.h"
#include "config.h"
#include "bot_commands.h"
#include "tg_debug.h"

using namespace std::chrono_literals;

namespace hanley_bot {

Bot::Bot(config::Config& config) :
	bot_(std::move(config.credentials.bot_token)),
	database_(config.credentials.database_uri),
	dialogs_(*this),
	config_(config.bot_config) {}

[[nodiscard]] bool Bot::IsOwner(domain::UserID user) const {
	return user == config_.owner_id;
}

[[nodiscard]] bool Bot::IsOwner(const TgBot::User::Ptr& user) const {
	return IsOwner(user->id);
}

[[nodiscard]] bool Bot::IsOwner(const TgBot::ChatMember::Ptr& user) const {
	return IsOwner(user->user);
}

[[nodiscard]] bool Bot::IsOwner(const TgBot::Message::Ptr& message) const {
	return IsOwner(message->from);
}

[[nodiscard]] bool Bot::IsMainGroup(const TgBot::Chat::Ptr& chat) const {
	return chat->id == config_.group_id;
}

[[nodiscard]] bool Bot::IsFromNewsThread(const TgBot::Message::Ptr& message) const {
	return IsMainGroup(message->chat) && message->messageThreadId == config_.news_thread_id;
}

void Bot::Run() {
	bot_.getEvents().onUnknownCommand([](TgBot::Message::Ptr message) {
		std::cout << "onUnknownCommand ";
		hanley_bot::tg::debug::DumpMessage(std::cout, message);
		std::cout << std::endl;
	});

	bot_.getEvents().onNonCommandMessage([this](TgBot::Message::Ptr message) {
		std::cout << "onNonCommandMessage ";
		hanley_bot::tg::debug::DumpMessage(std::cout, message);
		std::cout << std::endl;

		dialogs_.HandleTextInput(message);
	});

	bot_.getEvents().onInlineQuery([](TgBot::InlineQuery::Ptr) {
		//BOOST_LOG_TRIVIAL(warning) << "Unexpected event \"onInlineQuery\"!";
	});

	bot_.getEvents().onChosenInlineResult([](TgBot::ChosenInlineResult::Ptr) {
		//BOOST_LOG_TRIVIAL(warning) << "Unexpected event \"onChosenInlineResult\"!";
	});

	bot_.getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr query) {
		std::cout << "onCallbackQuery ";
		hanley_bot::tg::debug::DumpCallbackQuery(std::cout, query);
		std::cout << std::endl;

		try {
			if (query->data.starts_with("static_")) {
				commands::CallCommand(*this, query->data, query->message);
			} else {
				dialogs_.HandleCallback(query->message, query->data);
			}

			GetAPI().answerCallbackQuery(query->id);
		} catch (const TgBot::TgException& ex) {
			GetAPI().answerCallbackQuery(query->id, "Произошла ошибка при выполнении запроса. Если ошибка повторяется, напишите владельцу", true, "", 15);

			std::cout << "onCallbackQuery expection: " << ex.what() << std::endl;
		}
	});

	bot_.getEvents().onMyChatMember([](TgBot::ChatMemberUpdated::Ptr update) {
		std::cout << "onMyChatMember" << std::endl;
	});

	bot_.getEvents().onChatMember([](TgBot::ChatMemberUpdated::Ptr update) {
		std::cout << "onChatMember" << std::endl;
	});

	bot_.getEvents().onChatJoinRequest([](TgBot::ChatJoinRequest::Ptr update) {
		std::cout << "onChatJoinRequest" << std::endl;
	});

	//bot_.getApi().blockedByUser
	//bot_.getApi().copyMessage
	//bot_.getApi().deleteMessage
	//bot_.getApi().getChat
	//bot_.getApi().getChatMember
	//bot_.getApi().sendChatAction
	//bot_.getApi().sendMessage
	//bot_.getApi().banChatMember

	TgBot::TgLongPoll longPoll(bot_);

	while (true) {
		try {
			longPoll.start();
		} catch (const TgBot::TgException& e) {
			std::cout << "error: " << e.what();
			std::this_thread::sleep_for(3s);
		} catch (const std::exception& e) {
			std::cout << "(" << typeid(e).name() << ") error: " << e.what();
			std::this_thread::sleep_for(3s);
		} catch (...) {
			std::cout << "Unknown exception";
			std::this_thread::sleep_for(3s);
		}
	}
}

[[nodiscard]] pqxx::work Bot::BeginTransaction() {
	return pqxx::work{database_};
}

[[nodiscard]] const TgBot::Api& Bot::GetAPI() {
	return bot_.getApi();
}

[[nodiscard]] domain::UserID Bot::GetOwnerID() const {
	return config_.owner_id;
}

void Bot::RegisterCommand(const std::string& name, const TgBot::EventBroadcaster::MessageListener& listener) {
	bot_.getEvents().onCommand(name, listener);
}

TgBot::Message::Ptr Bot::SendMessage(domain::ChatID chat_id, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup, const std::string& parseMode, bool disableWebPagePreview,
	bool protectContent, bool disableNotification, domain::ThreadID thread_id) {

	return GetAPI().sendMessage(chat_id, text, disableWebPagePreview, 0, std::move(replyMarkup), parseMode, disableNotification, {}, false, protectContent, thread_id);
}

TgBot::Message::Ptr Bot::SendMessage(const TgBot::Message::Ptr& get_from_message, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup, const std::string& parseMode, bool disableWebPagePreview,
	bool protectContent, bool disableNotification) {

	return SendMessage(get_from_message->chat->id, text, std::move(replyMarkup), parseMode, disableWebPagePreview, protectContent, disableNotification, get_from_message->messageThreadId);
}

TgBot::Message::Ptr Bot::EditMessage(domain::ChatID chat_id, domain::MessageID message_id, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup,
	const std::string& parseMode, bool disableWebPagePreview) {

	return GetAPI().editMessageText(text, chat_id, message_id, "", parseMode, disableWebPagePreview, std::move(replyMarkup), {});
}

TgBot::Message::Ptr Bot::EditMessage(const TgBot::Message::Ptr& message_to_edit, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup,
	const std::string& parseMode, bool disableWebPagePreview) {

	return EditMessage(message_to_edit->chat->id, message_to_edit->messageId, text, std::move(replyMarkup), parseMode, disableWebPagePreview);
}

bool Bot::Typing(domain::ChatID chat_id) {
	return GetAPI().blockedByUser(chat_id);
}


} // namespace hanley_bot