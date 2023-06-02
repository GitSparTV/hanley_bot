#include <cassert>

#include <tgbot/types/Message.h>

#include "states_controller.h"
#include "state_machine.h"
#include "bot.h"

namespace hanley_bot::state {

void StateMachine::Link(StatesController& controller, TgBot::Message::Ptr message) {
	controller_ = &controller;
	message_ = std::move(message);
}

TgBot::Message::Ptr StateMachine::GetMessage() const {
	assert(message_);

	return message_;
}

hanley_bot::Bot& StateMachine::GetBot() {
	assert(controller_);

	return controller_->GetBot();
}

void StateMachine::ListenForInput() {
	assert(controller_);

	controller_->ListenForInput(GetMessage());
}

} // namespace hanley_bot::state