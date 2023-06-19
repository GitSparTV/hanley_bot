#include "sdk.h"

#include <chrono>
#include <thread>

#include <fmt/core.h>
#include <tgbot/tgbot.h>
#include <pqxx/pqxx>

#include "bot.h"
#include "domain.h"
#include "config.h"
#include "bot_commands.h"
#include "tg_debug.h"
#include "tg_utils.h"
#include "logging.h"
#include "currency_extension.h"

using namespace std::chrono_literals;

namespace hanley_bot {

Bot::Bot(config::Config& config) :
	bot_(std::move(config.credentials.bot_token)),
	database_(config.credentials.database_uri),
	dialogs_(*this),
	config_(std::move(config)) {}

[[nodiscard]] bool Bot::IsOwner(domain::UserID user) const {
	return GetOwnerID() == user;
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
	return chat->id == config_.bot_config.group_id;
}

[[nodiscard]] bool Bot::IsFromNewsThread(const TgBot::Message::Ptr& message) const {
	return IsMainGroup(message->chat) && message->messageThreadId == config_.bot_config.news_thread_id;
}

void Bot::Run() {
	bot_.getEvents().onUnknownCommand([](TgBot::Message::Ptr message) {
		LOG(warning) << "Received unknown command: " << message->text << " " << hanley_bot::tg::debug::DumpMessage(message);
	});

	bot_.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
		if (IsFromNewsThread(message) && !IsOwner(message)) {
			LOG(info) << "A message was posted in news thread, deleted: " << tg::debug::DumpMessage(message);

			GetAPI().deleteMessage(message->chat->id, message->messageId);
			return;
		}
	});

	bot_.getEvents().onNonCommandMessage([this](TgBot::Message::Ptr message) {
		if (tg::utils::IsPM(message->chat)) {
			LOG(info) << message->from->id << " wrote: \"" << message->text << "\"";
		}

		LOG(debug) << tg::debug::DumpMessage(message);

		try {
			dialogs_.HandleTextInput(message);
		} catch (const std::exception& ex) {
			LOG(error) << "Exception caught in HandleTextInput (" << typeid(ex).name() << "): " << ex.what();
		}

		EndTransaction();
	});

	bot_.getEvents().onInlineQuery([](TgBot::InlineQuery::Ptr) {
		LOG(warning) << "Received unhandled onInlineQuery event";
	});

	bot_.getEvents().onChosenInlineResult([](TgBot::ChosenInlineResult::Ptr) {
		LOG(warning) << "Received unhandled onChosenInlineResult event";
	});

	bot_.getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr query) {
		LOG(debug) << "Received onCallbackQuery: " << hanley_bot::tg::debug::DumpCallbackQuery(query);

		try {
			if (query->data.starts_with("static_")) {
				commands::CallCommand(*this, query->data, domain::Context::FromCallback(query));
			} else {
				dialogs_.HandleCallback(query->message, query->data);
			}

			AnswerCallbackQuery(query->id);
		} catch (const std::exception& ex) {
			try {
				AnswerCallbackQuery(query->id, "Произошла ошибка при выполнении запроса. Если ошибка повторяется, напишите владельцу", true, 15);
			} catch (std::exception& ex) {
				if (std::string_view(ex.what()).find("query is too old") != std::string_view::npos) {
					LOG_VERBOSE(warning) << "Query was too old. Answer was not sent";
					SendMessage(query->message, "Кажется, бот спал когда вы общались с ним. Повторите действие. Если ошибка повторяется, напишите владельцу");
				} else {
					LOG(error) << "Unexpected exception caught while calling AnswerCallbackQuery! " << ex.what();
					SendMessage(query->message, "Произошла ошибка при выполнении запроса. Если ошибка повторяется, напишите владельцу");
				}
			}

			LOG(error) << "Exception caught during callback query (" << typeid(ex).name() << "): " << ex.what();
		}

		EndTransaction();
	});

	bot_.getEvents().onMyChatMember([](TgBot::ChatMemberUpdated::Ptr) {
		LOG(warning) << "Received unhandled onMyChatMember event";
	});

	bot_.getEvents().onChatMember([](TgBot::ChatMemberUpdated::Ptr) {
		LOG(warning) << "Received unhandled onChatMember event";
	});

	bot_.getEvents().onChatJoinRequest([](TgBot::ChatJoinRequest::Ptr) {
		LOG(warning) << "Received unhandled onChatJoinRequest event";
	});

	TgBot::TgLongPoll long_poll(bot_);

	LOG(info) << "Starting polling...";

	while (true) {
		try {
			long_poll.start();
		} catch (const TgBot::TgException& ex) {
			LOG(error) << "TgExpection caught: " << ex.what();

			std::this_thread::sleep_for(3s);
		} catch (const std::exception& ex) {
			LOG(error) << "Exception caught (" << typeid(ex).name() << "): " << ex.what();

			std::this_thread::sleep_for(3s);
		} catch (...) {
			LOG(error) << "Unknown exception";

			std::this_thread::sleep_for(3s);
		}
	}
}

[[nodiscard]] pqxx::work& Bot::BeginTransaction() {
	if (current_transaction_) {
		return *current_transaction_;
	}

	LOG(debug) << "Begin transaction...";

	current_transaction_.emplace(database_);

	return *current_transaction_;
}

void Bot::Commit() {
	if (!current_transaction_) {
		LOG(warning) << "No transaction to commit";
	}

	LOG(debug) << "Commit transaction";

	current_transaction_->commit();
	current_transaction_.reset();
}

void Bot::EndTransaction() {
	if (!current_transaction_) {
		return;
	}

	LOG(debug) << "Force end transaction";

	current_transaction_.reset();
}

[[nodiscard]] const TgBot::Api& Bot::GetAPI() {
	return bot_.getApi();
}

[[nodiscard]] domain::UserID Bot::GetOwnerID() const {
	return config_.bot_config.owner_id;
}

void Bot::RegisterCommand(const std::string& name, const TgBot::EventBroadcaster::MessageListener& listener) {
	bot_.getEvents().onCommand(name, listener);
}

TgBot::Message::Ptr Bot::SendMessage(domain::ChatID chat_id, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup, const std::string& parseMode, bool disableWebPagePreview,
	bool protectContent, bool disableNotification, domain::ThreadID thread_id) {
	LOG_VERBOSE(trace) << fmt::format("SendMessage to {}. Length: {}.", chat_id, text.size());

	return GetAPI().sendMessage(chat_id, text, disableWebPagePreview, 0, std::move(replyMarkup), parseMode, disableNotification, {}, false, protectContent, thread_id);
}

TgBot::Message::Ptr Bot::SendMessage(const domain::Context& get_from_context, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup, const std::string& parseMode, bool disableWebPagePreview,
	bool protectContent, bool disableNotification) {

	return SendMessage(get_from_context.origin, text, std::move(replyMarkup), parseMode, disableWebPagePreview, protectContent, disableNotification, get_from_context.origin_thread);
}

TgBot::Message::Ptr Bot::SendMessage(const TgBot::Message::Ptr& get_from_message, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup, const std::string& parseMode, bool disableWebPagePreview,
	bool protectContent, bool disableNotification) {

	return SendMessage(get_from_message->chat->id, text, std::move(replyMarkup), parseMode, disableWebPagePreview, protectContent, disableNotification, get_from_message->messageThreadId);
}

TgBot::Message::Ptr Bot::EditMessage(domain::ChatID chat_id, domain::MessageID message_id, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup,
	const std::string& parseMode, bool disableWebPagePreview) {
	LOG_VERBOSE(trace) << fmt::format("EditMesage in {} id={}. Length: {}.", chat_id, message_id, text.size());

	return GetAPI().editMessageText(text, chat_id, message_id, "", parseMode, disableWebPagePreview, std::move(replyMarkup), {});
}

TgBot::Message::Ptr Bot::EditMessage(const TgBot::Message::Ptr& message_to_edit, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup,
	const std::string& parseMode, bool disableWebPagePreview) {

	return EditMessage(message_to_edit->chat->id, message_to_edit->messageId, text, std::move(replyMarkup), parseMode, disableWebPagePreview);
}

TgBot::Message::Ptr Bot::EditMessage(const domain::Context& message_to_edit, const std::string& text,
	TgBot::GenericReply::Ptr replyMarkup,
	const std::string& parseMode, bool disableWebPagePreview) {

	return EditMessage(message_to_edit.message, text, std::move(replyMarkup), parseMode, disableWebPagePreview);
}

bool Bot::Typing(domain::ChatID chat_id) {
	return GetAPI().blockedByUser(chat_id);
}

void Bot::AnswerCallbackQuery(const std::string& query_id, const std::string& text,
	bool show_alert, std::int32_t cache_time) {
	static std::string last_query_id;

	if (last_query_id == query_id) {
		LOG(debug) << "Query was already answered (" << query_id << ')';

		return;
	}

	last_query_id = query_id;

	LOG_VERBOSE(trace) << fmt::format("AnswerCallbackQuery {}. msg_len={}", query_id, text.size());

	GetAPI().answerCallbackQuery(query_id, text, show_alert, "", cache_time);
}

double Bot::GetRate() {
	auto& tx = BeginTransaction();

	auto result = tx.query01<double>(
		"DELETE FROM exchange_cache WHERE time < NOW() - INTERVAL '1 day'; "
		"SELECT rate FROM exchange_cache WHERE time > NOW() - INTERVAL '1 day';");

	if (result) {
		auto [rate] = *result;

		LOG_VERBOSE(trace) << "Got cached result";

		return rate;
	}

	LOG_VERBOSE(debug) << "Cache expired, getting new value...";
	double rate = currency::FetchConversionRate(config_.credentials.exchange_api);

	tx.exec0(fmt::format("INSERT INTO exchange_cache(rate) VALUES({});", rate));

	return rate;
}


} // namespace hanley_bot