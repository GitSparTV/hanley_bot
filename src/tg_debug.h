#pragma once

#include <iostream>

#include <tgbot/types/Message.h>
#include <tgbot/types/CallbackQuery.h>

namespace hanley_bot::tg::debug {

void DumpMessage(std::ostream& out, TgBot::Message::Ptr message);

void DumpCallbackQuery(std::ostream& out, TgBot::CallbackQuery::Ptr query);

} // namespace hanley_bot::tg::debug