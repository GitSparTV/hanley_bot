#include <pqxx/pqxx>

#include <cassert>
#include <charconv>
#include <string>
#include <vector>
#include <deque>
#include <format>

#include <tgbot/types/BotCommand.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/BotCommandScopeChat.h>
#include <tgbot/types/BotCommandScopeDefault.h>
#include <tgbot/types/BotCommandScopeAllGroupChats.h>

#include "bot_commands.h"
#include "bot.h"
#include "tg_utils.h"
#include "domain.h"

using namespace std::literals;

namespace hanley_bot::commands {

enum class Permission {
	kPublic,
	kPublicHidden,
	kOwner
};

struct CommandInfo {
	using Callback = void(*)(Bot&, const TgBot::Message::Ptr&, bool);

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

void RegisterUser(Bot& bot, pqxx::work& tx, TgBot::User::Ptr user) {
	if (tx.query_value<bool>(std::format("SELECT EXISTS(SELECT 1 FROM users WHERE telegram_id = {})", user->id))) {
		return;
	}

	tx.exec0(
		std::format("INSERT INTO users (telegram_id, first_name, last_name) VALUES ({}, '{}', NULLIF('{}','')) ON CONFLICT (telegram_id) DO UPDATE SET first_name = excluded.first_name, last_name = excluded.last_name;",
			user->id,
			tx.esc(user->firstName),
			tx.esc(user->lastName))
	);
}

void Start(Bot& bot, const TgBot::Message::Ptr& message, bool) {
	static constexpr std::string_view kStartCommand = "/start";

	const std::string& text = message->text;

	pqxx::work tx = bot.BeginTransaction();

	RegisterUser(bot, tx, message->from);

	bot.SendMessage(message, "Привет!\nЭто бот русскоязычной группы [\"Хенли. ПФА&ТОН\"](t.me/ruspfasbt).\nБот поможет зарегистрироваться на потоки обучения, а если их ещё нет, то подписаться на уведомления на те курсы, что вас интересуют.\nДля тех, кто уже учится на потоке, этот бот поможет вам активировать курс, получать новости группы и другое.\nСписок доступных всем команд есть у вас в меню внизу слева, если не видно, наберите \"/help\".\n\nВ случае неисправностей или вопросов пишите: t.me/savvatelegram.", {}, "Markdown");

	tx.commit();

	if (text.size() <= kStartCommand.size()) {
		return;
	}

	auto deep_link = text.substr(kStartCommand.size() + 1);

	if (!deep_link.starts_with("static")) {
		return;
	}

	CallCommand(bot, std::move(deep_link), message);
}

void GetCourses(Bot& bot, const TgBot::Message::Ptr& message, bool from_command) {
	static constexpr std::string_view kHeader = "Про все курсы и процесс сертификации можно почитать <a href=\"https://abasavva.notion.site/4b49aea6d6964c359039545d198ef7a2\">здесь</a>.\n";
	static constexpr size_t kAverageCourseNameLength = 30 * 2;
	static constexpr size_t kStartSize = kHeader.size() + kAverageCourseNameLength * 10;

	std::string result;

	result.reserve(kStartSize);
	result = kHeader;

	tg::utils::KeyboardBuilder keyboard;

	static constexpr int kButtonsPerRow = 3;
	auto row = keyboard.Row();

	pqxx::work tx = bot.BeginTransaction();

	for (const auto& [id, full_name, is_subscribed] : tx.query<uint64_t, std::string, bool>(std::format("SELECT id, full_name, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} ORDER BY courses.id ASC", message->from->id))) {
		result += std::format("\n<b>{}.</b> {}{}", id, full_name, is_subscribed ? " <i>(Вы подписаны на уведомления)</i>" : ""sv);

		row.Callback(std::to_string(id), std::format("static_courses_get_{}", id));

		if (id % kButtonsPerRow == 0) {
			row = keyboard.Row();
		}
	}

	if (from_command) {
		bot.SendMessage(message, result, keyboard, "HTML", true);
	} else {
		bot.EditMessage(message, result, keyboard, "HTML", true);
	}

	tx.commit();
}

void Test(Bot& bot, const TgBot::Message::Ptr& message, bool from_command) {
	//auto sent = bot.getApi().sendMessage(message->chat->id, "MainCourseForm test");

	//dialogs.Add<dialogs::MainCourseForm>(sent);
}

static const std::vector<CommandInfo> kCommands = {
	{"start", "Начать разговор", Start, Permission::kPublicHidden},
	{"courses", "Список курсов на русском языке от FTF", GetCourses, Permission::kPublic},
	{"test", "Тест", Test, Permission::kPublicHidden}
};

void PushCommands(Bot& bot) {
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

void InitializeCommands(Bot& bot) {
	for (const auto& command_info : kCommands) {
		auto listener = [&bot,
			permission = command_info.permission, callback = command_info.callback]
			(TgBot::Message::Ptr message) {

			if (permission == commands::Permission::kOwner && !bot.IsOwner(message)) {
				return;
			}

			return callback(bot, message, true);
		};

		bot.RegisterCommand(std::string(command_info.name), listener);
	}
}

std::deque<std::string_view> SplitPath(std::string_view path) {
	static constexpr char kSeparator = '_';

	std::deque<std::string_view> split_path;

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

struct StaticQueryInfo {
	using Callback = void(*)(Bot&, std::deque<std::string_view>&, const TgBot::Message::Ptr&, const TgBot::User::Ptr&);

	Callback callback;
	Permission permission;
};

void GetCourse(Bot& bot, const TgBot::Message::Ptr message_to_edit, domain::UserID user_id, std::string_view course_id) {
	int course_id_number;

	auto status = std::from_chars(course_id.data(), course_id.data() + course_id.size(), course_id_number);

	if (status.ec != std::errc{}) {
		return;
	}

	auto tx = bot.BeginTransaction();

	auto [full_name, description, url, is_subscribed] = tx.query1<std::string, std::string, std::string, bool>(std::format("SELECT full_name, description, url, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} WHERE id = {}", user_id, course_id_number));

	const std::string_view is_subscribed_text = is_subscribed ? "<i>Вы подписаны на новости об этом курсе.</i>" : "<i>Вы можете подписаться на новости об этом курсе ниже.</i>";

	std::string result = std::format("<b>{}</b>\n\n{}\n{}", full_name, description, is_subscribed_text);

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "static_courses_get"},
			{tg::utils::ButtonType::kLink, "🔗 Ссылка на курс", url}
		},
		{
			{tg::utils::ButtonType::kCallback,
			is_subscribed ? "Отписаться от новостей" : "Подписаться на новости",
			std::format("static_subs_{}_{}", is_subscribed ? "del" : "add", course_id)}
		}
	};

	bot.EditMessage(message_to_edit, result, keyboard, "HTML");

	tx.commit();
}

void Courses(Bot& bot, std::deque<std::string_view>& path,
	const TgBot::Message::Ptr& message, const TgBot::User::Ptr& user) {

	if (path.front() == "get") {
		path.pop_front();

		if (path.empty()) {
			return GetCourses(bot, message, false);
		}

		GetCourse(bot, message, user->id, path.front());
	}
}

static const std::unordered_map<std::string_view, StaticQueryInfo> kStaticQueries = {
	{"courses", {Courses, Permission::kPublic}},
};

void CallCommand(Bot& bot, std::string path, const TgBot::Message::Ptr& message, const TgBot::User::Ptr& user) {
	auto split_path = SplitPath(path);

	assert(split_path[0] == "static");

	split_path.pop_front();

	const auto entrypoint = kStaticQueries.find(split_path.front());

	if (entrypoint == std::end(kStaticQueries)) {
		return;
	}

	const auto& [callback, permission] = entrypoint->second;

	if (permission == Permission::kOwner && bot.IsOwner(user)) {
		return;
	}

	split_path.pop_front();

	callback(bot, split_path, message, user);
}

void CallCommand(Bot& bot, std::string path, const TgBot::Message::Ptr& message) {
	CallCommand(bot, std::move(path), message, message->from);
}

} // namespace hanley_bot::commands