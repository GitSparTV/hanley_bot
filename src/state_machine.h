#pragma once

#include <string_view>

#include <tgbot/types/Message.h>

#include "domain.h"

namespace hanley_bot {

class Bot;

} // namespace hanley_bot

namespace hanley_bot::state {

class StatesController;

class StateMachine {
public:
	virtual ~StateMachine() = default;

	virtual void EnterState() = 0;

	virtual void OnInput(const TgBot::Message::Ptr& input_message) = 0;

	virtual void OnCallback(std::string_view data) = 0;

public:
	void Link(StatesController& controller, const domain::Context& message);

	const domain::Context& GetContext() const;

	hanley_bot::Bot& GetBot();

	void ListenForInput();

private:
	StatesController* controller_ = nullptr;
	domain::Context context_;
};

} // namespace hanley_bot::state