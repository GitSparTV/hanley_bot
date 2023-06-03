#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <tgbot/tgbot.h>
#include <pqxx/pqxx>

#include "bot.h"
#include "config.h"
#include "bot_commands.h"
#include "tg_debug.h"

namespace hanley_bot {

Bot::Bot(config::Config& config) :
	bot_(std::move(config.credentials.bot_token)),
	database_(config.credentials.database_uri),
	dialogs_(*this),
	config_(config.bot_config) {}
//me_(bot_.getApi().getMe()->id) {}

bool Bot::IsOwner(const TgBot::User::Ptr& user) const {
	return user->id == config_.owner_id;
}

bool Bot::IsOwner(const TgBot::ChatMember::Ptr& user) const {
	return IsOwner(user->user);
}

bool Bot::IsOwner(const TgBot::Message::Ptr& message) const {
	return IsOwner(message->from);
}

bool Bot::IsMainGroup(const TgBot::Chat::Ptr& chat) const {
	return chat->id == config_.group_id;
}

bool Bot::IsFromNewsThread(const TgBot::Message::Ptr& message) const {
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
			if (dialogs_.HandleCallback(query->message, query->data)) {
				GetAPI().answerCallbackQuery(query->id, "", true, "", 0);
			}
		} catch (const TgBot::TgException& ex) {
			std::cout << ex.what() << std::endl;
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

	//pqxx::work tx{c};

	//std::string result = "<a href=\"tg://user?id=\">inline mention of a user</a> Доступные курсы:";

	//for (auto [full_name, url, description] : tx.query<std::string, std::string, std::string>("SELECT full_name, url, description FROM courses")) {
	//    result += "\n<b><a href=\"" + url + "\">" + full_name + "</a></b>\n" + description + '\n';
	//}

	//bot_.getApi().sendMessage(message->chat->id, result, false, 0, std::make_shared<TgBot::GenericReply>(), "HTML");

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
			//BOOST_LOG_TRIVIAL(info) << "error: " << e.what();
			std::this_thread::sleep_for(3s);
		} catch (const std::exception& e) {
			//BOOST_LOG_TRIVIAL(info) << "(" << typeid(e).name() << ") error: " << e.what();
		} catch (...) {
			//BOOST_LOG_TRIVIAL(info) << "Unknown exception";
		}
	}
}

pqxx::work Bot::MakeTransaction() {
	return pqxx::work{database_};
}

const TgBot::Api& Bot::GetAPI() {
	return bot_.getApi();
}

config::UserID Bot::GetOwnerID() const {
	return config_.owner_id;
}

void Bot::RegisterCommand(const std::string& name, const TgBot::EventBroadcaster::MessageListener& listener) {
	bot_.getEvents().onCommand(name, listener);
}


} // namespace hanley_bot