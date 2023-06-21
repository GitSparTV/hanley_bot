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

StatesController& StateMachine::GetController() {
	assert(controller_);

	return *controller_;
}

const domain::Context& StateMachine::GetContext() const {
	return context_;
}

hanley_bot::Bot& StateMachine::GetBot() {
	return GetController().GetBot();
}

void StateMachine::ListenForInput() {
	GetController().ListenForInput(GetContext());
}

void StateMachine::Finish() {
	GetController().Remove(GetContext());
}

} // namespace hanley_bot::state