#include "domain.h"

namespace hanley_bot::domain {

Context Context::FromCommand(const TgBot::Message::Ptr& message) {
	return {
		.user = message->from->id,
		.origin = message->chat->id,
		.origin_thread = message->messageThreadId,
		.type = InvokeType::kUserCommand,
		.message = message,
		.query_id{},
	};
}

Context Context::FromCallback(const TgBot::CallbackQuery::Ptr& query) {
	return {
		.user = query->from->id,
		.origin = query->message->chat->id,
		.origin_thread = query->message->messageThreadId,
		.type = InvokeType::kCallback,
		.message = query->message,
		.query_id = query->id,
	};
}

bool Context::IsUserCommand() const {
	return type == InvokeType::kUserCommand;
}

bool Context::IsCallback() const {
	return type == InvokeType::kCallback;
}

bool Context::IsCode() const {
	return type == InvokeType::kCode;
}

bool Context::IsPM() const {
	return origin == user;
}


} // namespace hanley_bot::domain