#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <tgbot/tgbot.h>
#include <pqxx/pqxx>

#include "bot.h"
#include "config.h"

namespace hanley_bot {

namespace commands {

enum class Permission {
	kPublic,
	kPublicHidden,
	kOwner
};

struct CommandInfo {
	using Callback = void(*)(const TgBot::Bot&, TgBot::Message::Ptr);

	std::string_view name;
	std::string_view description;
	Callback callback;
	Permission permission;
};

TgBot::BotCommand::Ptr MakeCommand(std::string_view name, std::string_view description) {
	auto command = std::make_shared<TgBot::BotCommand>();

	command->command = std::string(name);
	command->description = std::string(description);

	return command;
}

void RemoveUser(uint64_t user_id) {
	pqxx::work tx{c};

	tx.exec0(std::format("DELETE FROM users WHERE telegram_id = {}", user_id));

	tx.commit();

	std::cout << std::format("Removing user {} because he blocked the bot", user_id) << std::endl;
}

void Broadcast(TgBot::Message::Ptr original_message) {
	std::vector<std::pair<uint64_t, std::string>> users;

	pqxx::work tx{c};

	users.reserve(tx.query_value<size_t>("SELECT COUNT(*) FROM users"));

	for (auto& [id, name] : tx.query<uint64_t, std::string>("SELECT id, first_name FROM users")) {
		users.emplace_back(id, name);
	}

	tx.commit();

	std::vector<uint64_t> to_delete;

	for (const auto& [id, name] : users) {
		try {
			bot_instance.getApi().copyMessage(id, original_message->chat->id, original_message->messageId);
			std::this_thread::sleep_for(0.7s);
		} catch (const TgBot::TgException& ex) {
			if (ex.what() == "Forbidden: bot was blocked by the user") {
				RemoveUser(id);
			}
		}
	}
}

void RegisterUser(TgBot::User::Ptr user) {
	pqxx::work tx{c};

	tx.exec0(
		std::format("INSERT INTO users (telegram_id, first_name, last_name) VALUES ({}, '{}', NULLIF('{}','')) ON CONFLICT (telegram_id) DO UPDATE SET first_name = excluded.first_name, last_name = excluded.last_name;",
			user->id,
			tx.esc(user->firstName),
			tx.esc(user->lastName))
	);

	tx.commit();
}

void Start(const TgBot::Bot& bot_instance, TgBot::Message::Ptr message) {
	std::string_view deep_link = message->text;

	RegisterUser(message->from);

	if (deep_link.size() <= sizeof("/start ")) {
		std::cout << "No deep link" << std::endl;
		return;
	}

	deep_link = deep_link.substr(sizeof("/start"));

	std::cout << deep_link << std::endl;

	bot_instance.getApi().sendMessage(message->chat->id, "Selection test", true, 0);
}

void GetCourses(const TgBot::Bot& bot_instance, TgBot::Message::Ptr message) {
	if (bot_instance.getApi().blockedByUser(message->from->id)) {
		return;
	}

	pqxx::work tx{c};

	static constexpr std::string_view kHeader = "Про все курсы и процесс сертификации можно почитать <a href=\"https://abasavva.notion.site/4b49aea6d6964c359039545d198ef7a2\">здесь</a>.\n";
	static constexpr size_t kAverageCourseNameLength = 30 * 2;
	static constexpr size_t kStartSize = kHeader.size() + kAverageCourseNameLength * 10;

	std::string result;

	result.reserve(kStartSize);
	result = kHeader;

	hanley_bot::tg::utils::KeyboardBuilder keyboard;

	static constexpr int kButtonsPerRow = 3;
	auto row = keyboard.Row();

	for (const auto& [id, full_name, is_subscribed] : tx.query<uint64_t, std::string, bool>(std::format("SELECT id, full_name, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} ORDER BY courses.id ASC", message->from->id))) {
		result += std::format("\n<b>{}.</b> {}{}", id, full_name, is_subscribed ? " <i>(Вы подписаны на уведомления)</i>" : ""sv);

		row.Callback(std::to_string(id), std::format("static/courses/get/{}", id));

		if (id % kButtonsPerRow == 0) {
			row = keyboard.Row();
		}
	}

	tx.commit();

	bot_instance.getApi().sendMessage(message->chat->id, result, true, 0, keyboard, "HTML");
}

void Test(const TgBot::Bot& bot_instance, TgBot::Message::Ptr message) {
	auto sent = bot_instance.getApi().sendMessage(message->chat->id, "MainCourseForm test");

	dialogs.Add<hanley_bot::dialogs::MainCourseForm>(sent);
}

static const std::vector<CommandInfo> kCommands = {
	{"start"sv, "Start interaction with the bot"sv, Start, Permission::kPublicHidden},
	{"courses"sv, "Управление подписками на новости о курсах FTF", GetCourses, Permission::kPublic},
	{"test"sv, "Тест", Test, Permission::kPublicHidden}
};

} // namespace commands

Bot::Bot(config::Config& config) :
	bot_(std::move(config.credentials.bot_token)),
	database_(config.credentials.database_uri),
	dialogs_(*this),
	config_(config.bot_config) {}
//me_(bot_.getApi().getMe()->id) {}

void Bot::Run() {
	for (const auto& command_info : commands::kCommands) {
		bot_.getEvents().onCommand(std::string(command_info.name), [&command_info](TgBot::Message::Ptr message) {
			if (command_info.permission == commands::Permission::kOwner && !hanley_bot::tg::utils::IsOwner(message)) {
				return;
			}

			return command_info.callback(bot_, message);
		});
	}

	bot_.getEvents().onUnknownCommand([](TgBot::Message::Ptr message) {
		std::cout << "onUnknownCommand ";
		hanley_bot::tg::debug::DumpMessage(std::cout, message);
		std::cout << std::endl;
	});

	bot_.getEvents().onNonCommandMessage([](TgBot::Message::Ptr message) {
		std::cout << "onNonCommandMessage ";
		hanley_bot::tg::debug::DumpMessage(std::cout, message);
		std::cout << std::endl;

		dialogs.HandleTextInput(message);
	});

	bot_.getEvents().onInlineQuery([](TgBot::InlineQuery::Ptr) {
		BOOST_LOG_TRIVIAL(warning) << "Unexpected event \"onInlineQuery\"!";
	});

	bot_.getEvents().onChosenInlineResult([](TgBot::ChosenInlineResult::Ptr) {
		BOOST_LOG_TRIVIAL(warning) << "Unexpected event \"onChosenInlineResult\"!";
	});

	bot_.getEvents().onCallbackQuery([](TgBot::CallbackQuery::Ptr query) {
		std::cout << "onCallbackQuery ";
		hanley_bot::tg::debug::DumpCallbackQuery(std::cout, query);
		std::cout << std::endl;

		try {
			if (dialogs_.HandleCallback(query->message, query->data)) {
				bot_.getApi().answerCallbackQuery(query->id, "", true, "", 0);
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

	//p->inlineKeyboard.emplace_back(std::move(row0));

	//BOOST_LOG_TRIVIAL(info) << message->text;

	//if (message->text.starts_with("/")) {
	//    return;
	//}

	//pqxx::work tx{c};

	//std::string result = "<a href=\"tg://user?id=\">inline mention of a user</a> Доступные курсы:";

	//for (auto [full_name, url, description] : tx.query<std::string, std::string, std::string>("SELECT full_name, url, description FROM courses")) {
	//    result += "\n<b><a href=\"" + url + "\">" + full_name + "</a></b>\n" + description + '\n';
	//}

	//bot_.getApi().sendMessage(message->chat->id, result, false, 0, std::make_shared<TgBot::GenericReply>(), "HTML");

//bot_.getEvents().onCommand("start", [&bot_, &keyboard](Message::Ptr message) {
//    bot_.getApi().sendMessage(message->chat->id, "Hi!", false, 0, keyboard);
//});

//bot_.getEvents().onCommand("check", [&bot_, &keyboard](Message::Ptr message) {
//    string response = "ok";
//    bot_.getApi().sendMessage(message->chat->id, response, false, 0, keyboard, "Markdown");
//});

//bot_.getEvents().onCallbackQuery([&bot_, &keyboard](CallbackQuery::Ptr query) {
//    if (StringTools::startsWith(query->data, "check")) {
//        string response = "ok";
//        bot_.getApi().sendMessage(query->message->chat->id, response, false, 0, keyboard, "Markdown");
//    }
//});

	//bot_.getApi().blockedByUser
	//bot_.getApi().copyMessage
	//bot_.getApi().deleteMessage
	//bot_.getApi().getChat
	//bot_.getApi().getChatMember
	//bot_.getApi().sendChatAction
	//bot_.getApi().sendMessage
	//bot_.getApi().banChatMember

	try {
		InitializeCommands(bot_);
	} catch (TgBot::TgException& e) {
		BOOST_LOG_TRIVIAL(info) << "error: " << e.what();
		std::this_thread::sleep_for(3s);
	}

	BOOST_LOG_TRIVIAL(info) << "Bot username: " << bot_.getApi().getMe()->username;

	TgBot::TgLongPoll longPoll(bot_);
	while (true) {
		try {
			longPoll.start();
		} catch (const TgBot::TgException& e) {
			BOOST_LOG_TRIVIAL(info) << "error: " << e.what();
			std::this_thread::sleep_for(3s);
		} catch (const std::exception& e) {
			BOOST_LOG_TRIVIAL(info) << "(" << typeid(e).name() << ") error: " << e.what();
		} catch (...) {
			BOOST_LOG_TRIVIAL(info) << "Unknown exception";
		}
	}
}

void InitializeCommands() {
	std::vector<TgBot::BotCommand::Ptr> owner_commands;
	owner_commands.reserve(commands::kCommands.size());

	std::vector<TgBot::BotCommand::Ptr> public_commands;

	for (const auto& command_info : commands::kCommands) {
		auto command = commands::MakeCommand(command_info.name, command_info.description);

		if (command_info.permission == commands::Permission::kPublic) {
			public_commands.emplace_back(command);
		}

		owner_commands.emplace_back(command);
	}

	{
		auto owner_chat = std::make_shared<TgBot::BotCommandScopeChat>();
		owner_chat->chatId = 0;

		bot_instance.getApi().setMyCommands(owner_commands, owner_chat);
	}

	bot_instance.getApi().setMyCommands(public_commands, std::make_shared<TgBot::BotCommandScopeDefault>());

	bot_instance.getApi().setMyCommands({}, std::make_shared<TgBot::BotCommandScopeAllGroupChats>());
}

} // namespace hanley_bot