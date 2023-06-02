#include <vector>
#include <tuple>
#include <string>
#include <memory>
#include <functional>

#include <tgbot/types/InlineKeyboardMarkup.h>
#include <tgbot/types/User.h>
#include <tgbot/types/ChatMember.h>
#include <tgbot/types/Message.h>

#include "tg_utils.h"

namespace hanley_bot::tg::utils {

bool IsPM(TgBot::Chat::Ptr chat) {
    return chat->type == TgBot::Chat::Type::Private;
}

bool IsOwner(TgBot::User::Ptr user) {
    return user->id == 0;
}

bool IsOwner(TgBot::ChatMember::Ptr user) {
    return IsOwner(user->user);
}

bool IsOwner(TgBot::Message::Ptr message) {
    return IsOwner(message->from);
}

MakeKeyboard::MakeKeyboard(std::initializer_list<std::vector<std::tuple<ButtonType, std::string_view, std::string_view>>> args) {
    inlineKeyboard.reserve(args.size());

    for (const auto& internal_row : args) {
        auto& row = inlineKeyboard.emplace_back();
        row.reserve(internal_row.size());

        for (const auto& [type, name, data] : internal_row) {
            auto button = row.emplace_back(std::make_shared<TgBot::InlineKeyboardButton>());

            button->text = std::string(name);

            if (type == ButtonType::kCallback) {
                button->callbackData = std::string(data);
            } else {
                button->url = std::string(data);
            }
        }
    }

}

MakeKeyboard::operator TgBot::GenericReply::Ptr() {
    auto shared = std::make_shared<TgBot::InlineKeyboardMarkup>();

    shared->inlineKeyboard = std::move(inlineKeyboard);

    return shared;
}

KeyboardBuilder::OneRow::OneRow(KeyboardBuilder::OneRow::Underlying& buttons, KeyboardBuilder& parent) : buttons_(buttons), parent_(parent) {}

KeyboardBuilder::OneRow& KeyboardBuilder::OneRow::Insert(TgBot::InlineKeyboardButton&& button) {
    buttons_.get().emplace_back(std::make_shared<TgBot::InlineKeyboardButton>(std::move(button)));

    return *this;
}

KeyboardBuilder::OneRow& KeyboardBuilder::OneRow::Callback(std::string_view text, std::string_view data) {
    TgBot::InlineKeyboardButton button = {
        .text = std::string(text),
        .callbackData = std::string(data)
    };

    return Insert(std::move(button));
}

KeyboardBuilder::OneRow& KeyboardBuilder::OneRow::Link(std::string_view text, std::string_view link) {
    TgBot::InlineKeyboardButton button = {
        .text = std::string(text),
        .url = std::string(link)
    };

    return Insert(std::move(button));
}

KeyboardBuilder& KeyboardBuilder::OneRow::End() {
    return parent_;
}

KeyboardBuilder::OneRow KeyboardBuilder::Row() {
    return {inlineKeyboard.emplace_back(), *this};
}

KeyboardBuilder::operator TgBot::GenericReply::Ptr() {
    auto shared = std::make_shared<TgBot::InlineKeyboardMarkup>();

    shared->inlineKeyboard = std::move(inlineKeyboard);

    return shared;
}

} // namespace hanley_bot::tg::utils