#include <pqxx/pqxx>

#include <cassert>
#include <charconv>
#include <string>
#include <vector>
#include <deque>

#include <fmt/core.h>
#include <tgbot/types/BotCommand.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/BotCommandScopeChat.h>
#include <tgbot/types/BotCommandScopeDefault.h>
#include <tgbot/types/BotCommandScopeAllGroupChats.h>

#include "logging.h"
#include "bot_commands.h"
#include "bot.h"
#include "tg_utils.h"
#include "tg_debug.h"
#include "domain.h"

using namespace std::literals;

namespace hanley_bot::commands {

enum class Permission {
	kPublic,
	kPublicHidden,
	kOwner
};

struct CommandInfo {
	using Callback = void(*)(Bot&, const domain::Context&);

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

bool ParseNumber(std::string_view text, int& output) {
	auto status = std::from_chars(text.data(), text.data() + text.size(), output);

	return status.ec == std::errc{};
}

std::string_view GetCommandArgument(std::string_view text) {
	return text.substr(text.find_first_of(' ') + 1);
}

void RemoveUser(uint64_t user_id) {
	//pqxx::work tx{c};

	//tx.exec0(fmt::format("DELETE FROM users WHERE telegram_id = {}", user_id));

	//tx.commit();

	//std::cout << fmt::format("Removing user {} because he blocked the bot", user_id) << std::endl;
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

bool RegisterUser(Bot& bot, pqxx::work& tx, TgBot::User::Ptr user) {
	if (tx.query_value<bool>(fmt::format("SELECT EXISTS(SELECT 1 FROM users WHERE telegram_id = {})", user->id))) {
		LOG_VERBOSE(debug) << "User " << user->id << " already registered";

		return false;
	}

	tx.exec0(
		fmt::format("INSERT INTO users (telegram_id, first_name, last_name) VALUES ({}, '{}', NULLIF('{}','')) ON CONFLICT (telegram_id) DO UPDATE SET first_name = excluded.first_name, last_name = excluded.last_name;",
			user->id,
			tx.esc(user->firstName),
			tx.esc(user->lastName))
	);

	LOG_VERBOSE(debug) << "New user: " << user->id << " was registered";

	return true;
}

void Start(Bot& bot, const domain::Context& context) {
	if (!context.IsUserCommand() || !context.IsPM()) {
		LOG_VERBOSE(warning) << "Invoked from invalid context: " << tg::debug::DumpContext(context);

		return;
	}

	static constexpr std::string_view kStartCommand = "/start";

	const std::string& text = context.message->text;
	bool has_deep_link = text.size() <= kStartCommand.size();

	auto& tx = bot.BeginTransaction();
	bool already_registered = RegisterUser(bot, tx, context.message->from);

	if (!already_registered || (already_registered && has_deep_link)) {
		bot.SendMessage(context.message, "Привет!\nЭто бот русскоязычной группы [\"Хенли. ПФА&ТОН\"](t.me/ruspfasbt).\nБот поможет зарегистрироваться на потоки обучения, а если их ещё нет, то подписаться на уведомления на те курсы, что вас интересуют.\nДля тех, кто уже учится на потоке, этот бот поможет вам активировать курс, получать новости группы и другое.\nСписок доступных всем команд есть у вас в меню внизу слева, если не видно, наберите \"/help\".\n\nВ случае неисправностей или вопросов пишите: t.me/savvatelegram.", {}, "Markdown");
	}

	tx.commit();

	if (has_deep_link) {
		LOG_VERBOSE(debug) << "No deep link";
		return;
	}

	auto deep_link = text.substr(kStartCommand.size() + 1);

	if (!deep_link.starts_with("static")) {
		LOG_VERBOSE(warning) << "Deep link doesn't start with word \"static\". Content: " << text;

		return;
	}

	CallCommand(bot, std::move(deep_link), context);
}

void GetCourses(Bot& bot, const domain::Context& context) {
	if (!context.IsPM()) {
		LOG_VERBOSE(warning) << "Invoked from invalid context: " << tg::debug::DumpContext(context);

		return;
	}

	static constexpr std::string_view kHeader = "Про все курсы и процесс сертификации можно почитать <a href=\"https://abasavva.notion.site/4b49aea6d6964c359039545d198ef7a2\">здесь</a>.\n";
	static constexpr size_t kAverageCourseNameLength = 30 * 2;
	static constexpr size_t kStartSize = kHeader.size() + kAverageCourseNameLength * 10;

	std::string result;

	result.reserve(kStartSize);
	result = kHeader;

	tg::utils::KeyboardBuilder keyboard;

	static constexpr int kButtonsPerRow = 3;
	auto row = keyboard.Row();

	auto& tx = bot.BeginTransaction();

	for (const auto& [id, full_name, is_subscribed] : tx.query<uint64_t, std::string, bool>(fmt::format("SELECT id, full_name, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} ORDER BY courses.id ASC", context.user))) {
		result += fmt::format("\n<b>{}.</b> {}{}", id, full_name, is_subscribed ? " <i>(Вы подписаны на уведомления)</i>" : ""sv);

		row.Callback(std::to_string(id), fmt::format("static_courses_get_{}", id));

		if (id % kButtonsPerRow == 0) {
			row = keyboard.Row();
		}
	}

	if (context.IsUserCommand()) {
		bot.SendMessage(context, result, keyboard, "HTML", true);
	} else {
		bot.EditMessage(context, result, keyboard, "HTML", true);
	}

	tx.commit();
}

extern const std::vector<CommandInfo> kCommands;

void Help(Bot& bot, const domain::Context& context) {
	if (!context.IsUserCommand() || !context.IsPM()) {
		LOG_VERBOSE(warning) << "Invoked from invalid context: " << tg::debug::DumpContext(context);

		return;
	}

	std::string result = "В случае неисправностей пишите [мне](t.me/savvatelegram)\n\nДоступные команды:\n";

	for (const auto& command : kCommands) {
		if (command.permission == Permission::kPublic || bot.IsOwner(context.user)) {
			result += fmt::format("/{} — {}\n", command.name, command.description);
		}
	}

	bot.SendMessage(context, result, {}, "Markdown", true);
}

void LogLevel(Bot& bot, const domain::Context& context) {
	auto severity = GetCommandArgument(context.message->text);

	LOG(warning) << "Severity was changed to " << severity;
	
	logger::ChangeSeverityFilter(severity);

	bot.SendMessage(context, fmt::format("Changed severity to {}", severity));
	//auto sent = bot.getApi().sendMessage(message->chat->id, "MainCourseForm test");

	//dialogs.Add<dialogs::MainCourseForm>(sent);
}

const std::vector<CommandInfo> kCommands = {
	{"start", "Начать разговор", Start, Permission::kPublicHidden},
	{"help", "Помощь", Help, Permission::kPublic},
	{"courses", "Список курсов на русском языке от FTF", GetCourses, Permission::kPublic},
	{"loglevel", "Change Boost.Log severity", LogLevel, Permission::kOwner}
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
		auto listener = [&bot, &command_info]
		(TgBot::Message::Ptr message) {
			LOG(info) << fmt::format("User {} invoked /{} in {}", message->from->id, command_info.name, message->chat->id);

			if (command_info.permission == commands::Permission::kOwner && !bot.IsOwner(message)) {
				LOG_VERBOSE(warning) << "Access denied. " << tg::debug::DumpMessage(message);

				return;
			}

			command_info.callback(bot, domain::Context::FromCommand(message));

			bot.EndTransaction();
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
	using Callback = void(*)(Bot&, std::deque<std::string_view>&, const domain::Context&);

	Callback callback;
	Permission permission;
};

void GetCourse(Bot& bot, const domain::Context& context, std::string_view course_id) {
	int course_id_number;

	if (!ParseNumber(course_id, course_id_number)) {
		LOG_VERBOSE(error) << "Invalid course_id (" << course_id << ')';

		return;
	}

	auto& tx = bot.BeginTransaction();

	auto [full_name, description, url, is_subscribed] = tx.query1<std::string, std::string, std::string, bool>(fmt::format("SELECT full_name, description, url, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} WHERE id = {}", context.user, course_id_number));

	const std::string_view is_subscribed_text = is_subscribed ? "<i>Вы подписаны на новости об этом курсе.</i>" : "<i>Вы можете подписаться на новости об этом курсе ниже.</i>";

	LOG_VERBOSE(debug) << fmt::format("GetCourse: course={}, user={}, is_subscribed={}", full_name, context.user, is_subscribed);

	std::string result = fmt::format("<b>{}</b>\n\n{}\n{}", full_name, description, is_subscribed_text);

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "static_courses_get"},
			{tg::utils::ButtonType::kLink, "🔗 Ссылка на курс", url}
		},
		{
			{tg::utils::ButtonType::kCallback,
			is_subscribed ? "🔕 Отписаться от новостей" : "🔔 Подписаться на новости",
			fmt::format("static_subs_{}_{}", is_subscribed ? "del" : "add", course_id)}
		}
	};

	if (context.IsCallback()) {
		bot.EditMessage(context, result, keyboard, "HTML");
	} else {
		bot.SendMessage(context, result, keyboard, "HTML");
	}

	tx.commit();
}

void Courses(Bot& bot, std::deque<std::string_view>& path, const domain::Context& context) {
	if (path.front() == "get") {
		path.pop_front();

		if (path.empty()) {
			return GetCourses(bot, context);
		}

		GetCourse(bot, context, path.front());
	}
}

void Subscriptions(Bot& bot, std::deque<std::string_view>& path, const domain::Context& context) {
	const auto verb = path.front();

	path.pop_front();

	const auto course_id = path.front();
	int course_id_number;

	if (!ParseNumber(course_id, course_id_number)) {
		LOG_VERBOSE(error) << "Invalid course_id (" << course_id << ')';

		return;
	}

	std::string response;

	auto& tx = bot.BeginTransaction();

	const auto full_name = tx.query_value<std::string>(fmt::format("SELECT full_name FROM courses WHERE id = {}", course_id_number));

	if (verb == "add") {
		tx.exec0(fmt::format("INSERT INTO subscriptions (telegram_id, course_id) VALUES ({}, {}) ON CONFLICT DO NOTHING", context.user, course_id_number));

		response = fmt::format("Вы подписались на новости о курсе \"{}\"", full_name);

		LOG_VERBOSE(info) << fmt::format("Subscribed {} to {}", context.user, course_id);
	} else if (verb == "del") {
		tx.exec0(fmt::format("DELETE FROM subscriptions WHERE telegram_id = {} AND course_id = {}", context.user, course_id_number));

		response = fmt::format("Вы больше не будете получать новости о курсе \"{}\"", full_name);

		LOG_VERBOSE(info) << fmt::format("Unsubscribed {} from {}", context.user, course_id);
	} else {
		LOG_VERBOSE(error) << fmt::format("Subscriptions: Unknown verb ({}) {}", verb, tg::debug::DumpContext(context));

		response = "Ошибка. Напишите владельцу.";
	}

	if (context.IsCallback()) {
		bot.AnswerCallbackQuery(std::string(context.query_id), response, true, 2);

		tx.commit();

		return GetCourse(bot, context, course_id);
	}

	bot.SendMessage(context, response);

	tx.commit();
}

static const std::unordered_map<std::string_view, StaticQueryInfo> kStaticQueries = {
	{"courses", {Courses, Permission::kPublic}},
	{"subs", {Subscriptions, Permission::kPublic}}
};

void CallCommand(Bot& bot, std::string path, const domain::Context& context) {
	auto split_path = SplitPath(path);

	assert(split_path[0] == "static");
	LOG_VERBOSE(info) << fmt::format("Command invoked: {}", path);

	split_path.pop_front();

	const auto entrypoint = kStaticQueries.find(split_path.front());

	if (entrypoint == std::end(kStaticQueries)) {
		LOG_VERBOSE(error) << fmt::format("Unknown entrypoint {} (full path: {})", split_path.front(), path);

		return;
	}

	const auto& [callback, permission] = entrypoint->second;

	if (permission == Permission::kOwner && bot.IsOwner(context.user)) {
		LOG_VERBOSE(warning) << fmt::format("Access denied. path={} context={}", path, tg::debug::DumpContext(context));

		return;
	}

	split_path.pop_front();

	callback(bot, split_path, context);
}

} // namespace hanley_bot::commands