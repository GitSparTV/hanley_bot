#pragma once

#include <vector>
#include <tuple>
#include <string>
#include <memory>
#include <functional>

#include <tgbot/types/InlineKeyboardMarkup.h>
#include <tgbot/types/GenericReply.h>
#include <tgbot/types/Chat.h>

namespace hanley_bot::tg::utils {

bool IsPM(const TgBot::Chat::Ptr& chat);

enum class ButtonType {
	kCallback,
	kLink
};

struct MakeKeyboard : private TgBot::InlineKeyboardMarkup {
	explicit(false) MakeKeyboard(std::initializer_list<std::vector<std::tuple<ButtonType, std::string_view, std::string_view>>> args);

	explicit(false) operator TgBot::GenericReply::Ptr();
};

struct KeyboardBuilder : private TgBot::InlineKeyboardMarkup {
	class OneRow {
	public:
		using Underlying = decltype(InlineKeyboardMarkup::inlineKeyboard)::value_type;

	public:
		OneRow(Underlying& buttons, KeyboardBuilder& parent);

	public:
		OneRow& Insert(TgBot::InlineKeyboardButton&& button);

		OneRow& Callback(std::string_view text, std::string_view data);

		OneRow& Link(std::string_view text, std::string_view link);

		KeyboardBuilder& End();

	private:
		std::reference_wrapper<Underlying> buttons_;
		std::reference_wrapper<KeyboardBuilder> parent_;
	};

	OneRow Row();

	explicit(false) operator TgBot::GenericReply::Ptr();
};

} // namespace hanley_bot::tg::utils