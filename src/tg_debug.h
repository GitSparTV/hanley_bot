#pragma once

#include <tgbot/types/Message.h>
#include <tgbot/types/CallbackQuery.h>

namespace hanley_bot::tg::debug {

std::string DumpMessage(TgBot::Message::Ptr message);

std::string DumpCallbackQuery(TgBot::CallbackQuery::Ptr query);

} // namespace hanley_bot::tg::debug