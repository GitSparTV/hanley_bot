#include "domain.h"

namespace hanley_bot::domain {

Context Context::FromCommand(const TgBot::Message::Ptr& message) {
	return {
		.from = message->from->id,
		.origin = message->chat->id,
		.origin_thread = message->messageThreadId,
		.message = message->messageId,
		.content = message->text,
		.type = InvokeType::kUserCommand,
	};
}

Context Context::FromCallback(const TgBot::Message::Ptr& message, UserID user_id, const std::string& query_id) {
	return {
		.from = user_id,
		.origin = message->chat->id,
		.origin_thread = message->messageThreadId,
		.message = message->messageId,
		.content = query_id,
		.type = InvokeType::kCallback,
	};
}

} // namespace hanley_bot::domain