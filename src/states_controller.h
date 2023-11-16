#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>

#include <tgbot/types/Message.h>

#include "domain.h"

namespace hanley_bot {

class Bot;

} // namespace hanley_bot

namespace hanley_bot::state {

class StateMachine;

class StatesController {
public:
	explicit StatesController(hanley_bot::Bot& bot);

	void CollectGargabe();

	void CheckForGarbage();

	template<typename T, typename... Args>
	void Add(const domain::Context& context, Args&&... args) {
		CheckForGarbage();

		auto [machine_it, _] = machines_[context.message->chat->id].emplace(
			context.message->messageId,
			std::pair{std::make_shared<T>(), std::chrono::system_clock::now()}
		);

		auto& [key, machine_info] = *machine_it;
		auto& [machine, timestamp] = machine_info;

		machine->Link(*this, context);

		auto machine_downcasted = std::static_pointer_cast<T>(machine);

		if constexpr (sizeof...(args) != 0) {
			machine_downcasted->ReceiveArgs(std::forward<Args>(args)...);
		}

		machine_downcasted->EnterState();
	}

	void Remove(const TgBot::Message::Ptr& message);

	void Remove(const domain::Context& context);

	std::shared_ptr<StateMachine>& GetMachine(const TgBot::Message::Ptr& message);

	std::weak_ptr<StateMachine>& GetListener(const TgBot::Message::Ptr& message);

	bool HandleTextInput(TgBot::Message::Ptr input_message);

	bool HandleCallback(const TgBot::Message::Ptr& message, std::string_view data);

	void ListenForInput(const domain::Context& context);

	void ListenForInput(const TgBot::Message::Ptr& message);

	void StopListeningForInput(const domain::Context& context);

	void StopListeningForInput(const TgBot::Message::Ptr& message);

	hanley_bot::Bot& GetBot();

private:
	using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;

	std::unordered_map<hanley_bot::domain::ChatID, std::unordered_map<hanley_bot::domain::MessageID, std::pair<std::shared_ptr<StateMachine>, TimeStamp>>> machines_;
	std::unordered_map<hanley_bot::domain::ChatID, std::weak_ptr<StateMachine>> input_listeners_;

	hanley_bot::Bot& bot_;

	int garbage_trigger_ = 0;
	static constexpr int kGarbageThreshold = 10;
};

} // namespace hanley_bot::state