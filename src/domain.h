#pragma once

#include <string_view>

#include <tgbot/types/User.h>
#include <tgbot/types/Chat.h>
#include <tgbot/types/Message.h>
#include <tgbot/types/CallbackQuery.h>

namespace hanley_bot::domain {

using UserID = decltype(TgBot::User::id);
using ChatID = decltype(TgBot::Chat::id);
using ThreadID = decltype(TgBot::Message::messageThreadId);
using MessageID = decltype(TgBot::Message::messageId);

enum class InvokeType {
	kUserCommand,
	kCallback,
	kCode
};

struct Context {
	static Context FromCommand(const TgBot::Message::Ptr& message);
	static Context FromCallback(const TgBot::CallbackQuery::Ptr& query);

	bool IsUserCommand() const;
	bool IsCallback() const;
	bool IsCode() const;
	bool IsPM() const;

	UserID user = 0;
	ChatID origin = 0;
	ThreadID origin_thread = 0;
	InvokeType type = InvokeType::kUserCommand;
	TgBot::Message::Ptr message;
	std::string_view query_id;
};

} // namespace hanley_bot::domain