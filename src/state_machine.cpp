#include <cassert>

#include <tgbot/types/Message.h>

#include "states_controller.h"
#include "state_machine.h"
#include "bot.h"

namespace hanley_bot::state {

void StateMachine::Link(StatesController& controller, const domain::Context& context) {
	controller_ = &controller;
	context_ = context;
}

const domain::Context& StateMachine::GetContext() const {
	return context_;
}

hanley_bot::Bot& StateMachine::GetBot() {
	assert(controller_);

	return controller_->GetBot();
}

void StateMachine::ListenForInput() {
	assert(controller_);

	controller_->ListenForInput(GetContext());
}

} // namespace hanley_bot::state