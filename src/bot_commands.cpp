#include <pqxx/pqxx>

#include <cassert>
#include <string>
#include <vector>

#include <tgbot/types/BotCommand.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/BotCommandScopeChat.h>
#include <tgbot/types/BotCommandScopeDefault.h>
#include <tgbot/types/BotCommandScopeAllGroupChats.h>

#include "bot_commands.h"
#include "bot.h"

namespace hanley_bot::commands {

enum class Permission {
	kPublic,
	kPublicHidden,
	kOwner
};

struct CommandInfo {
	using Callback = void(*)(hanley_bot::Bot&, TgBot::Message::Ptr);

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
	//pqxx::work tx{c};

	//tx.exec0(std::format("DELETE FROM users WHERE telegram_id = {}", user_id));

	//tx.commit();

	//std::cout << std::format("Removing user {} because he blocked the bot", user_id) << std::endl;
}

void Broadcast(TgBot::Message::Ptr original_message) {
	//std::vector<std::pair<uint64_t, std::string>> users;

	//pqxx::work tx{c};

	//users.reserve(tx.query_value<size_t>("SELECT COUNT(*) FROM users"));

	//for (auto& [id, name] : tx.query<uint64_t, std::string>("SELECT id, first_name FROM users")) {
	//	users.emplace_back(id, name);
	//}

	//tx.commit();

	//std::vector<uint64_t> to_delete;

	//for (const auto& [id, name] : users) {
	//	try {
	//		bot.getApi().copyMessage(id, original_message->chat->id, original_message->messageId);
	//		std::this_thread::sleep_for(0.7s);
	//	} catch (const TgBot::TgException& ex) {
	//		if (ex.what() == "Forbidden: bot was blocked by the user") {
	//			RemoveUser(id);
	//		}
	//	}
	//}
}

void RegisterUser(TgBot::User::Ptr user) {
	//pqxx::work tx{c};

	//tx.exec0(
	//	std::format("INSERT INTO users (telegram_id, first_name, last_name) VALUES ({}, '{}', NULLIF('{}','')) ON CONFLICT (telegram_id) DO UPDATE SET first_name = excluded.first_name, last_name = excluded.last_name;",
	//		user->id,
	//		tx.esc(user->firstName),
	//		tx.esc(user->lastName))
	//);

	//tx.commit();
}

void Start(hanley_bot::Bot& bot, TgBot::Message::Ptr message) {
	//std::string_view deep_link = message->text;

	//RegisterUser(message->from);

	//if (deep_link.size() <= sizeof("/start ")) {
	//	std::cout << "No deep link" << std::endl;
	//	return;
	//}

	//deep_link = deep_link.substr(sizeof("/start"));

	//std::cout << deep_link << std::endl;

	//bot.getApi().sendMessage(message->chat->id, "Selection test", true, 0);
}

void GetCourses(hanley_bot::Bot& bot, TgBot::Message::Ptr message) {
	//if (bot.getApi().blockedByUser(message->from->id)) {
	//	return;
	//}

	//pqxx::work tx{c};

	//static constexpr std::string_view kHeader = "Про все курсы и процесс сертификации можно почитать <a href=\"https://abasavva.notion.site/4b49aea6d6964c359039545d198ef7a2\">здесь</a>.\n";
	//static constexpr size_t kAverageCourseNameLength = 30 * 2;
	//static constexpr size_t kStartSize = kHeader.size() + kAverageCourseNameLength * 10;

	//std::string result;

	//result.reserve(kStartSize);
	//result = kHeader;

	//hanley_bot::tg::utils::KeyboardBuilder keyboard;

	//static constexpr int kButtonsPerRow = 3;
	//auto row = keyboard.Row();

	//for (const auto& [id, full_name, is_subscribed] : tx.query<uint64_t, std::string, bool>(std::format("SELECT id, full_name, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} ORDER BY courses.id ASC", message->from->id))) {
	//	result += std::format("\n<b>{}.</b> {}{}", id, full_name, is_subscribed ? " <i>(Вы подписаны на уведомления)</i>" : ""sv);

	//	row.Callback(std::to_string(id), std::format("static/courses/get/{}", id));

	//	if (id % kButtonsPerRow == 0) {
	//		row = keyboard.Row();
	//	}
	//}

	//tx.commit();

	//bot.getApi().sendMessage(message->chat->id, result, true, 0, keyboard, "HTML");
}

void Test(hanley_bot::Bot& bot, TgBot::Message::Ptr message) {
	//auto sent = bot.getApi().sendMessage(message->chat->id, "MainCourseForm test");

	//dialogs.Add<hanley_bot::dialogs::MainCourseForm>(sent);
}

static const std::vector<CommandInfo> kCommands = {
	{"start", "", Start, Permission::kPublicHidden},
	{"courses", "Список курсов на русском языке от FTF", GetCourses, Permission::kPublic},
	{"test", "Тест", Test, Permission::kPublicHidden}
};

void PushCommands(hanley_bot::Bot& bot) {
	std::vector<TgBot::BotCommand::Ptr> owner_commands;
	owner_commands.reserve(kCommands.size());

	std::vector<TgBot::BotCommand::Ptr> public_commands;

	for (const auto& command_info : kCommands) {
		auto command = MakeCommand(command_info.name, command_info.description);

		if (command_info.permission == Permission::kPublic) {
			public_commands.emplace_back(command);
		}

		owner_commands.emplace_back(command);
	}

	{
		auto owner_chat = std::make_shared<TgBot::BotCommandScopeChat>();
		owner_chat->chatId = bot.GetOwnerID();

		bot.GetAPI().setMyCommands(owner_commands, owner_chat);
	}

	bot.GetAPI().setMyCommands(public_commands, std::make_shared<TgBot::BotCommandScopeDefault>());

	bot.GetAPI().setMyCommands({}, std::make_shared<TgBot::BotCommandScopeAllGroupChats>());
}

void InitializeCommands(hanley_bot::Bot& bot) {
	for (const auto& command_info : kCommands) {
		auto listener = [&bot,
			permission = command_info.permission, callback = command_info.callback]
			(TgBot::Message::Ptr message) {

			if (permission == commands::Permission::kOwner && !bot.IsOwner(message)) {
				return;
			}

			return callback(bot, message);
		};

		bot.RegisterCommand(std::string(command_info.name), listener);
	}
}

std::vector<std::string_view> SplitPath(std::string_view path) {
	static constexpr char kSeparator = '/';

	std::vector<std::string_view> split_path;

	while (true) {
		auto separator = path.find_first_of(kSeparator);

		auto part = path.substr(0, separator);

		if (!part.empty()) {
			split_path.emplace_back(part);
		}

		path = path.substr(separator + 1);

		if (separator == std::string::npos) {
			break;
		}
	}

	return split_path;
}

void CallCommand(hanley_bot::Bot& bot, std::string path, const TgBot::Message::Ptr& message) {
	auto split_path = SplitPath(path);

	assert(split_path[0] == "static");
}

} // namespace hanley_bot::commands