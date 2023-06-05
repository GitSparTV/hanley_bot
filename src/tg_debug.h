#pragma once

#include <tgbot/types/Message.h>
#include <tgbot/types/CallbackQuery.h>

#include "domain.h"

namespace hanley_bot::tg::debug {

std::string DumpMessage(const TgBot::Message::Ptr& message);

std::string DumpCallbackQuery(const TgBot::CallbackQuery::Ptr& query);

std::string DumpContext(const hanley_bot::domain::Context& context);

} // namespace hanley_bot::tg::debug