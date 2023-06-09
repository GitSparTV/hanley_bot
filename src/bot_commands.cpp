#include <cassert>
#include <charconv>
#include <string>
#include <vector>
#include <deque>
#include <thread>

#include "sdk.h"
#include <pqxx/pqxx>
#include <fmt/format.h>
#include <tgbot/types/BotCommand.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/BotCommandScopeChat.h>
#include <tgbot/types/BotCommandScopeDefault.h>
#include <tgbot/types/BotCommandScopeAllGroupChats.h>
#include <boost/json.hpp>

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
	auto pos = text.find_first_of(' ');

	if (pos == std::string_view::npos) {
		return {};
	}

	return text.substr(pos + 1);
}

void RemoveUser(pqxx::work& tx, uint64_t user_id) {
	tx.exec0(fmt::format("DELETE FROM users WHERE telegram_id = {}", user_id));

	LOG_VERBOSE(info) << fmt::format("Removing user {} because he blocked the bot", user_id);
}

void BroadcastAll(Bot& bot, const domain::Context& context) {
	if (!context.message) {
		LOG_VERBOSE(error) << "Broadcast called from unsupported context";

		return;
	}

	if (!context.message->replyToMessage) {
		bot.SendMessage(context, "Command must be called with replied message");

		return;
	}

	auto& tx = bot.BeginTransaction();

	for (auto& [id, name] : tx.query<uint64_t, std::string>("SELECT telegram_id, first_name FROM users")) {
		try {
			bot.GetAPI().copyMessage(id, context.message->replyToMessage->chat->id, context.message->replyToMessage->messageId);

			std::this_thread::sleep_for(0.7s);
		} catch (const TgBot::TgException& ex) {
			if (ex.what() == "Forbidden: bot was blocked by the user"sv) {
				RemoveUser(tx, id);
			}
		}
	}
}

void BroadcastSubscriptionsTest(Bot& bot, [[maybe_unused]] const domain::Context& context) {
	auto& tx = bot.BeginTransaction();

	std::unordered_map<uint64_t, std::string> course_id_to_name;

	for (auto [course_id, full_name] : tx.query<uint64_t, std::string>("SELECT id, full_name FROM courses")) {
		course_id_to_name.emplace(course_id, std::move(full_name));
	}

	for (auto [id, courses_json] : tx.query<uint64_t, std::string>("SELECT users.telegram_id, JSON_AGG(subscriptions.course_id) as subscriptions FROM users LEFT JOIN subscriptions ON subscriptions.telegram_id = users.telegram_id GROUP BY users.telegram_id")) {
		const auto json_value = boost::json::parse(courses_json);
		const auto& courses_json_array = json_value.as_array();

		std::vector<int64_t> courses;
		courses.reserve(courses_json_array.size());

		for (const auto& course_id_json : courses_json_array) {
			if (course_id_json.is_int64()) {
				courses.emplace_back(course_id_json.get_int64());
			}
		}

		std::string result;

		if (courses.empty()) {
			result = "Привет!\nЕсли ты читаешь это сообщение, значит бот вас помнит.\n\nОднако бот не нашёл у тебя ни одной подписки на курс. :(\nЕсли это ошибка, напиши боту /courses и подпишись на интересующий курс";
		} else {
			result = "Привет!\nЕсли ты читаешь это сообщение, значит бот вас помнит. Проверь, что бот правильно запомнил какие курсы тебя интересуют:\n";

			for (auto course_id : courses) {
				result += fmt::format("\n<b>-</b> {}", course_id_to_name[course_id]);
			}

			result += "\n\nЕсли список неверный, напиши боту /courses и подпишись на интересующий курс";
		}

		try {
			bot.SendMessage(id, result, {}, "HTML");

			std::this_thread::sleep_for(0.5s);
		} catch (const TgBot::TgException& ex) {
			if (ex.what() == "Forbidden: bot was blocked by the user"sv) {
				RemoveUser(tx, id);
			}
		}
	}

	bot.SendMessage(bot.GetOwnerID(), "Done");
	bot.Commit();
}

bool RegisterUser(pqxx::work& tx, TgBot::User::Ptr user) {
	if (tx.query_value<bool>(fmt::format("SELECT EXISTS(SELECT 1 FROM users WHERE telegram_id = {})", user->id))) {
		LOG_VERBOSE(debug) << "User " << user->id << " already registered";

		return false;
	}

	tx.exec0(
		fmt::format("INSERT INTO users (telegram_id, first_name, last_name) VALUES ({}, '{}', NULLIF('{}',''))",
			user->id,
			tx.esc(user->firstName),
			tx.esc(user->lastName))
	);

	LOG_VERBOSE(info) << "New user: " << user->id << " was registered";

	return true;
}

void Start(Bot& bot, const domain::Context& context) {
	if (!context.IsUserCommand() || !context.IsPM()) {
		LOG_VERBOSE(warning) << "Invoked from invalid context: " << tg::debug::DumpContext(context);

		return;
	}

	const std::string& text = context.message->text;
	auto deep_link = GetCommandArgument(text);

	auto& tx = bot.BeginTransaction();
	bool registered = RegisterUser(tx, context.message->from);

	if (registered || (!registered && deep_link.empty())) {
		bot.SendMessage(context.message, "Привет!\nЭто бот русскоязычной группы [\"Хенли. ПФА&ТОН\"](t.me/ruspfasbt).\n*Что он умеет?*\n\nБот поможет зарегистрироваться на потоки обучения, а если их ещё нет, то подписаться на уведомления на те курсы, что вас интересуют.\nДля тех, кто уже учится на потоке, этот бот поможет вам активировать курс, получать новости группы и другое.\n\n*Как пользоваться ботом?*\n\nНачните с ним общение в этом чате.\nПосмотрите в нижний левый угол, там будет синяя кнопка-меню (☰) с доступными командами. Если не видите, напишите */help* (Можно нажать прямо здесь)\nНа данный момент доступна только подписка на новости :)\n\n_ℹ️ Организация курсов делается безвозмездно, во благо популяризации и развития подхода в русскоязычном сообществе._\n\nЕсли увидели неисправность, хотите поблагодарить, пожертвовать или задать вопрос, напишите [мне](t.me/savvatelegram).", {}, "Markdown", true);
	}

	bot.Commit();

	if (deep_link.empty()) {
		LOG_VERBOSE(debug) << "No deep link";
		return;
	}

	if (!deep_link.starts_with("static")) {
		LOG_VERBOSE(warning) << "Deep link doesn't start with word \"static\". Content: " << text;

		return;
	}

	CallCommand(bot, std::string(deep_link), context);
}

void GetCourses(Bot& bot, const domain::Context& context) {
	if (!context.IsPM()) {
		LOG_VERBOSE(warning) << "Invoked from invalid context: " << tg::debug::DumpContext(context);

		return;
	}

	constexpr std::string_view kHeader = "Про все курсы и процесс сертификации можно почитать <a href=\"https://abasavva.notion.site/4b49aea6d6964c359039545d198ef7a2\">здесь</a>.\n";
	constexpr size_t kAverageCourseNameLength = 30ULL * 2ULL;
	constexpr size_t kStartSize = kHeader.size() + kAverageCourseNameLength * 10ULL;

	std::string result;

	result.reserve(kStartSize);
	result = kHeader;

	tg::utils::KeyboardBuilder keyboard;

	constexpr int kButtonsPerRow = 3;
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

	bot.Commit();
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

void Statistics(Bot& bot, const domain::Context& context) {
	auto& tx = bot.BeginTransaction();

	auto user_count = tx.query_value<uint64_t>("SELECT COUNT(id) FROM users");

	std::string result = fmt::format("**Statistics**\nUser count: {}\n", user_count);

	for (auto [short_name, count] : tx.query<std::string, uint64_t>("SELECT c.short_name, COUNT(s.telegram_id) FROM courses AS c LEFT JOIN subscriptions AS s ON c.id = s.course_id GROUP BY c.id, c.short_name ORDER BY c.id ASC")) {
		result += fmt::format("\n**{}:** {}", short_name, count);
	}

	bot.SendMessage(context, result, {}, "Markdown");
}

const std::vector<CommandInfo> kCommands = {
	{"start", "Начать разговор", Start, Permission::kPublicHidden},
	{"help", "Помощь", Help, Permission::kPublic},
	{"courses", "Список курсов на русском языке от FTF", GetCourses, Permission::kPublic},
	{"loglevel", "Change Boost.Log severity", LogLevel, Permission::kOwner},
	{"stats", "Statistics", Statistics, Permission::kOwner},
	{"broadcast", "Broadcast message to everyone", BroadcastAll, Permission::kOwner},
	{"broadcast_s", "Broadcast their subscriptions", BroadcastSubscriptionsTest, Permission::kOwner},
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
	constexpr char kSeparator = '_';

	std::deque<std::string_view> split_path;

	while (true) {
		auto separator = path.find_first_of(kSeparator);

		if (auto part = path.substr(0, separator); !part.empty()) {
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

std::string Pad3Digits(int number) {
	std::string serialized = fmt::format_int(number).str();

	auto size = serialized.size();

	if (size <= 4) {
		return serialized;
	}

	auto [div, rem] = std::lldiv(static_cast<long long>(size), 3LL);

	auto new_size = size + (static_cast<size_t>(div) - (rem ? 0 : 1));

	serialized.reserve(new_size);

	for (size_t s = (rem ? static_cast<size_t>(rem) : 3ULL), offset = 0ULL; s != size; s += 3ULL, ++offset) {
		serialized.insert(s + offset, 1, ' ');
	}

	return serialized;
}

void InsertExchanges(Bot& bot, std::string& text) {
	size_t last_pos = 0;

	double rate = bot.GetRate();

	while (true) {
		auto pos = text.find_first_of('$', last_pos);

		if (pos == std::string::npos) {
			return;
		}

		int price;

		auto piece = std::string_view(text).substr(pos + 1);

		auto status = std::from_chars(piece.data(), piece.data() + piece.size(), price);

		if (status.ec == std::errc{}) {
			auto converted_padded_price = Pad3Digits(static_cast<int>(price * rate));

			text.insert(static_cast<size_t>(status.ptr - text.data()),
				fmt::format(" (Примерно {}₽ на текущий день)", converted_padded_price));
		}

		last_pos = pos + 1;
	}
}

void GetCourse(Bot& bot, const domain::Context& context, std::string_view course_id) {
	int course_id_number;

	if (!ParseNumber(course_id, course_id_number)) {
		LOG_VERBOSE(error) << "Invalid course_id (" << course_id << ')';

		return;
	}

	auto& tx = bot.BeginTransaction();

	auto [full_name, description, url, is_subscribed] = tx.query1<std::string, std::string, std::string, bool>(fmt::format("SELECT full_name, description, url, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {} WHERE id = {}", context.user, course_id_number));

	const std::string_view is_subscribed_text = is_subscribed ? "<i>ℹ️ Вы подписаны на новости об этом курсе.</i>" : "<i>ℹ️ Вы можете подписаться на новости об этом курсе ниже.</i>";

	LOG_VERBOSE(debug) << fmt::format("GetCourse: course={}, user={}, is_subscribed={}", full_name, context.user, is_subscribed);

	InsertExchanges(bot, description);

	std::string result = fmt::format("<b>{}</b>\n\n{}\n\n{}", full_name, description, is_subscribed_text);

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
		bot.EditMessage(context, result, keyboard, "HTML", true);
	} else {
		bot.SendMessage(context, result, keyboard, "HTML", true);
	}

	bot.Commit();
}

void Courses(Bot& bot, std::deque<std::string_view>& path, const domain::Context& context) {
	if (path.empty()) {
		LOG_VERBOSE(error) << "Invalid path. No verb";

		return;
	}

	if (path.front() == "get") {
		path.pop_front();

		if (path.empty()) {
			return GetCourses(bot, context);
		}

		GetCourse(bot, context, path.front());
	}
}

void Subscriptions(Bot& bot, std::deque<std::string_view>& path, const domain::Context& context) {
	if (path.empty()) {
		LOG_VERBOSE(error) << "Invalid path. No verb";

		return;
	}

	const auto verb = path.front();

	path.pop_front();

	if (path.empty()) {
		LOG_VERBOSE(error) << "Invalid path. No course_id";

		return;
	}

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

		bot.Commit();

		return GetCourse(bot, context, course_id);
	}

	bot.SendMessage(context, response);

	bot.Commit();
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

	if (split_path.empty()) {
		LOG_VERBOSE(error) << fmt::format("Entrypoint is empty (full path: {})", path);

		return;
	}

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