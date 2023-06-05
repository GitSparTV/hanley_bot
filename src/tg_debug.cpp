#include <format>
#include <string>
#include <unordered_map>
#include <sstream>

#include <tgbot/types/Message.h>
#include <tgbot/types/MessageEntity.h>
#include <tgbot/types/CallbackQuery.h>

#include "tg_debug.h"
#include "tg_utils.h"

namespace hanley_bot::tg::debug {

using namespace std::literals;

static const std::unordered_map<TgBot::MessageEntity::Type, std::string_view> kMessageEntititySerialized = {
	{TgBot::MessageEntity::Type::Mention, "Mention"sv},
	{TgBot::MessageEntity::Type::Hashtag, "Hashtag"sv},
	{TgBot::MessageEntity::Type::Cashtag, "Cashtag"sv},
	{TgBot::MessageEntity::Type::BotCommand, "BotCommand"sv},
	{TgBot::MessageEntity::Type::Url, "Url"sv},
	{TgBot::MessageEntity::Type::Email, "Email"sv},
	{TgBot::MessageEntity::Type::PhoneNumber, "PhoneNumber"sv},
	{TgBot::MessageEntity::Type::Bold, "Bold"sv},
	{TgBot::MessageEntity::Type::Italic, "Italic"sv},
	{TgBot::MessageEntity::Type::Underline, "Underline"sv},
	{TgBot::MessageEntity::Type::Strikethrough, "Strikethrough"sv},
	{TgBot::MessageEntity::Type::Spoiler, "Spoiler"sv},
	{TgBot::MessageEntity::Type::Code, "Code"sv},
	{TgBot::MessageEntity::Type::Pre, "Pre"sv},
	{TgBot::MessageEntity::Type::TextLink, "TextLink"sv},
	{TgBot::MessageEntity::Type::TextMention, "TextMention"sv},
	{TgBot::MessageEntity::Type::CustomEmoji, "CustomEmoji"sv}
};

std::string DumpMessage(TgBot::Message::Ptr message) {
	std::vector<std::string> attributes;
	attributes.reserve(58);

	auto emplace_func = [&attributes](std::string_view name, const auto& attribute, const auto& method) {
		if (attribute) {
			attributes.emplace_back(std::format("{}[{}]", name, std::invoke(method, *attribute)));
		}
	};

	auto emplace_self = [&attributes](std::string_view name, const auto& attribute) {
		if (attribute) {
			attributes.emplace_back(std::format("{}[{}]", name, attribute));
		}
	};

	auto emplace_name = [&attributes](std::string_view name, const auto& attribute) {
		if (attribute) {
			attributes.emplace_back(std::format("{}", name));
		}
	};

	auto emplace_non_empty = [&attributes](std::string_view name, const auto& attribute) {
		if (!attribute.empty()) {
			attributes.emplace_back(std::format("{}[{}]", name, attribute));
		}
	};

	auto emplace_size = [&attributes](std::string_view name, const auto& attribute) {
		if (!attribute.empty()) {
			attributes.emplace_back(std::format("{}[size:{}]", name, attribute.size()));
		}
	};

	auto emplace_id = [emplace_func](std::string_view name, const auto& attribute) {
		emplace_func(name, attribute, &std::decay_t<decltype(*attribute)>::id);
	};

	emplace_id("forwardFrom", message->forwardFrom);
	emplace_id("forwardFromChat", message->forwardFromChat);
	emplace_self("forwardFromMessageId", message->forwardFromMessageId);
	emplace_non_empty("forwardSignature", message->forwardSignature);
	emplace_non_empty("forwardSenderName", message->forwardSenderName);
	emplace_self("forwardDate", message->forwardDate);
	emplace_name("isTopicMessage", message->isTopicMessage);
	emplace_name("isAutomaticForward", message->isAutomaticForward);
	emplace_func("replyToMessage", message->replyToMessage, &std::decay_t<decltype(*message->replyToMessage)>::messageId);
	emplace_id("viaBot", message->viaBot);
	emplace_self("editDate", message->editDate);
	emplace_name("hasProtectedContent", message->hasProtectedContent);
	emplace_non_empty("mediaGroupId", message->mediaGroupId);
	emplace_non_empty("authorSignature", message->authorSignature);
	emplace_name("text", !message->text.empty());

	if (!message->entities.empty()) {
		std::string entities = "entities["s;

		bool is_first = true;
		for (const auto& i : message->entities) {
			if (!is_first) {
				entities += ", "s;
			}
			is_first = false;

			entities += kMessageEntititySerialized.at(i->type);
		}

		entities += "]"s;

		attributes.emplace_back(std::move(entities));
	}

	emplace_func("animation", message->animation, &std::decay_t<decltype(*message->animation)>::fileName);
	emplace_func("audio", message->audio, &std::decay_t<decltype(*message->audio)>::fileName);
	emplace_func("document", message->document, &std::decay_t<decltype(*message->document)>::fileName);
	emplace_size("photo", message->photo);
	emplace_func("sticker", message->sticker, &std::decay_t<decltype(*message->sticker)>::emoji);
	emplace_func("video", message->video, &std::decay_t<decltype(*message->video)>::fileName);
	emplace_func("videoNote", message->videoNote, &std::decay_t<decltype(*message->videoNote)>::fileId);
	emplace_func("voice", message->voice, &std::decay_t<decltype(*message->voice)>::fileId);
	emplace_name("caption", !message->caption.empty());

	if (!message->captionEntities.empty()) {
		std::string captionEntities = "captionEntities["s;

		bool is_first = true;
		for (const auto& i : message->captionEntities) {
			if (!is_first) {
				captionEntities += ", "s;
			}
			is_first = false;

			captionEntities += kMessageEntititySerialized.at(i->type);
		}

		captionEntities += "]"s;

		attributes.emplace_back(std::move(captionEntities));
	}

	emplace_func("contact", message->contact, &std::decay_t<decltype(*message->contact)>::phoneNumber);

	if (message->dice) {
		attributes.emplace_back(std::format("dice[{}:{}]", message->dice->emoji, message->dice->value));
	}

	emplace_func("game", message->game, &std::decay_t<decltype(*message->game)>::title);
	emplace_id("poll", message->poll);
	emplace_func("venue", message->venue, &std::decay_t<decltype(*message->venue)>::title);

	if (message->location) {
		attributes.emplace_back(std::format("location[{}, {}]", message->location->latitude, message->location->longitude));
	}

	emplace_size("newChatMembers", message->newChatMembers);
	emplace_id("leftChatMember", message->leftChatMember);
	emplace_non_empty("newChatTitle", message->newChatTitle);
	emplace_size("newChatPhoto", message->newChatPhoto);
	emplace_name("deleteChatPhoto", message->deleteChatPhoto);
	emplace_name("groupChatCreated", message->groupChatCreated);
	emplace_name("supergroupChatCreated", message->supergroupChatCreated);
	emplace_name("channelChatCreated", message->channelChatCreated);
	emplace_func("messageAutoDeleteTimerChanged", message->messageAutoDeleteTimerChanged, &std::decay_t<decltype(*message->messageAutoDeleteTimerChanged)>::messageAutoDeleteTime);
	emplace_self("migrateToChatId", message->migrateToChatId);
	emplace_self("migrateFromChatId", message->migrateFromChatId);
	emplace_func("pinnedMessage", message->pinnedMessage, &std::decay_t<decltype(*message->pinnedMessage)>::messageId);
	emplace_name("invoice", message->invoice);
	emplace_name("successfulPayment", message->successfulPayment);
	emplace_name("connectedWebsite", !message->connectedWebsite.empty());
	emplace_name("passportData", message->passportData);
	emplace_name("proximityAlertTriggered", message->proximityAlertTriggered);
	emplace_name("forumTopicCreated", message->forumTopicCreated);
	emplace_name("forumTopicClosed", message->forumTopicClosed);
	emplace_name("forumTopicReopened", message->forumTopicReopened);
	emplace_name("videoChatScheduled", message->videoChatScheduled);
	emplace_name("videoChatStarted", message->videoChatStarted);
	emplace_name("videoChatEnded", message->videoChatEnded);
	emplace_name("videoChatParticipantsInvited", message->videoChatParticipantsInvited);
	emplace_name("webAppData", message->webAppData);

	if (message->replyMarkup) {
		attributes.emplace_back(std::format("replyMarkup[{}]", message->replyMarkup->inlineKeyboard.size()));
	}

	std::ostringstream out;

	out << "Message["sv << message->messageId << "] in ["sv << message->chat->id << "]->["sv << (message->senderChat ? message->senderChat->id : 0) << "]->["sv << message->messageThreadId << "] ("sv << (hanley_bot::tg::utils::IsPM(message->chat) ? "private"sv : "group"sv) << ") from ["sv << message->from->id << "]. "sv << std::max(message->text.size(), message->caption.size()) << " characters. ["sv;

	bool is_first = true;

	for (const auto& attribute : attributes) {
		if (!is_first) {
			out << ", "sv;
		}
		is_first = false;

		out << attribute;
	}

	out << "]"sv;

	return std::move(out).str();
}

std::string DumpCallbackQuery(TgBot::CallbackQuery::Ptr query) {
	std::ostringstream out;

	out << "CallbackQuery["sv << query->id << "] in "sv << query->chatInstance << " from "sv << query->from->id << ", data: "sv << query->data;

	if (!query->inlineMessageId.empty()) {
		out << ", inlineMessageId: "sv << query->inlineMessageId;
	}

	if (!query->gameShortName.empty()) {
		out << ", gameName: "sv << query->gameShortName;
	}

	out << "\t[REFERENCE] ";

	out << DumpMessage(query->message);

	return std::move(out).str();
}

} // namespace hanley_bot::tg::debug