#pragma once

#include <string>

#include <tgbot/types/Message.h>

#include "domain.h"
#include "bot.h"

namespace hanley_bot::commands {

void PushCommands(hanley_bot::Bot& bot);
void InitializeCommands(hanley_bot::Bot& bot);
void CallCommand(hanley_bot::Bot& bot, std::string path, const domain::Context& context);

} //namespace hanley_bot::commands
