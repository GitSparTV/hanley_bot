#pragma once

#include <string>

#include <tgbot/types/User.h>
#include <tgbot/types/Chat.h>
#include <tgbot/types/Message.h>

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
	static Context FromCallback(const TgBot::Message::Ptr& message, UserID user_id, const std::string& query_id);

	UserID from = 0;
	ChatID origin = 0;
	ThreadID origin_thread = 0;
	MessageID message = 0;
	std::string content;
	InvokeType type = InvokeType::kUserCommand;
};

} // namespace hanley_bot::domain