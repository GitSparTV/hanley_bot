#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>

#include <tgbot/types/Message.h>

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

	template<typename T>
	void Add(TgBot::Message::Ptr message) {
		CheckForGarbage();

		auto [machine_it, _] = machines_[message->chat->id].emplace(
			message->messageId,
			std::pair{std::make_shared<T>(), std::chrono::system_clock::now()}
		);

		auto& [key, machine_info] = *machine_it;
		auto& [machine, timestamp] = machine_info;

		machine->Link(*this, message);

		auto machine_downcasted = std::static_pointer_cast<T>(machine);

		machine_downcasted->EnterState();
	}

	void Remove(const TgBot::Message::Ptr& message);

	std::shared_ptr<StateMachine>& GetMachine(const TgBot::Message::Ptr& message);

	void HandleTextInput(TgBot::Message::Ptr input_message);

	bool HandleCallback(const TgBot::Message::Ptr& message, std::string_view data);

	void ListenForInput(const TgBot::Message::Ptr& message);

	hanley_bot::Bot& GetBot();

private:
	using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;

	std::unordered_map<int64_t, std::unordered_map<int64_t, std::pair<std::shared_ptr<StateMachine>, TimeStamp>>> machines_;
	std::unordered_map<int64_t, std::weak_ptr<StateMachine>> input_listeners_;

	hanley_bot::Bot& bot_;

	int garbage_trigger_ = 0;
	static constexpr int kGarbageThreshold = 10;
};

} // namespace hanley_bot::state