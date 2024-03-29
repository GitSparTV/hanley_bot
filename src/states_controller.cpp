#include <cassert>
#include <chrono>
#include <memory>
#include <unordered_map>

#include "state_machine.h"
#include "states_controller.h"
#include "bot.h"

namespace hanley_bot::state {

StatesController::StatesController(hanley_bot::Bot& bot) : bot_(bot) {}

void StatesController::CollectGargabe() {
	auto chat_it = std::begin(machines_);
	auto chat_end_it = std::end(machines_);

	auto now = std::chrono::system_clock::now();

	while (chat_it != chat_end_it) {
		auto& chat_machines = chat_it->second;

		auto message_it = std::begin(chat_machines);
		auto message_end_it = std::end(chat_machines);

		while (message_it != message_end_it) {
			const auto& [_, machine_info] = *message_it;
			const auto& [machine, timestamp] = machine_info;

			if (std::chrono::days{2} < now - timestamp) {
				message_it = chat_machines.erase(message_it);
			} else {
				++message_it;
			}
		}

		if (chat_machines.empty()) {
			chat_it = machines_.erase(chat_it);
		} else {
			++chat_it;
		}
	}
}

void StatesController::CheckForGarbage() {
	++garbage_trigger_;

	if (garbage_trigger_ == kGarbageThreshold) {
		garbage_trigger_ = 0;

		CollectGargabe();
	}
}

void StatesController::Remove(const TgBot::Message::Ptr& message) {
	auto chat_it = machines_.find(message->chat->id);

	if (chat_it == std::end(machines_)) {
		return;
	}

	auto& chat_machines = chat_it->second;

	chat_machines.erase(message->messageId);
}

void StatesController::Remove(const domain::Context& context) {
	Remove(context.message);
}

std::shared_ptr<StateMachine>& StatesController::GetMachine(const TgBot::Message::Ptr& message) {
	static std::shared_ptr<StateMachine> empty;

	auto chat_it = machines_.find(message->chat->id);

	if (chat_it == std::end(machines_)) {
		return empty;
	}

	auto& chat_machines = chat_it->second;

	auto message_it = chat_machines.find(message->messageId);

	if (message_it == std::end(chat_machines)) {
		return empty;
	}

	auto& [machine, timestamp] = message_it->second;

	return machine;
}

std::weak_ptr<StateMachine>& StatesController::GetListener(const TgBot::Message::Ptr& message) {
	static std::weak_ptr<StateMachine> empty;

	auto listener_it = input_listeners_.find(message->chat->id);

	if (listener_it == std::end(input_listeners_)) {
		return empty;
	}

	auto& listener = listener_it->second;

	return listener;
}

bool StatesController::HandleTextInput(TgBot::Message::Ptr input_message) {
	const auto& listener = GetListener(input_message);

	if (listener.expired()) {
		return false;
	}

	auto machine = listener.lock();
	assert(machine);

	StopListeningForInput(input_message);

	machine->OnInput(input_message);

	return true;
}

bool StatesController::HandleCallback(const TgBot::Message::Ptr& message, std::string_view data) {
	const auto& machine = GetMachine(message);

	if (!machine) {
		return false;
	}

	machine->OnCallback(data);

	return true;
}

void StatesController::ListenForInput(const TgBot::Message::Ptr& message) {
	const auto& machine = GetMachine(message);

	if (!machine) {
		return;
	}

	input_listeners_.insert_or_assign(message->chat->id, machine);
}

void StatesController::ListenForInput(const domain::Context& context) {
	ListenForInput(context.message);
}

void StatesController::StopListeningForInput(const TgBot::Message::Ptr& message) {
	input_listeners_.erase(message->chat->id);
}

void StatesController::StopListeningForInput(const domain::Context& context) {
	StopListeningForInput(context.message);
}

hanley_bot::Bot& StatesController::GetBot() {
	return bot_;
}

} // namespace hanley_bot::state