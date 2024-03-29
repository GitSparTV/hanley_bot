#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef NDEBUG
#include <iostream>
#endif // !NDEBUG

#include <tgbot/types/Message.h>

#include "logging.h"
#include "state_base.h"
#include "state_machine.h"

namespace hanley_bot::state {

namespace utils {

template<typename Enum, typename Machine>
concept StateOf = std::is_same_v<typename Machine::State, Enum>;

template<typename Machine>
struct FactoryCreator {
public:
	using ReturnType = typename Machine::StatePtr;
	using State = typename Machine::State;
	using FactoryType = ReturnType(*)();

	static std::vector<FactoryType>& GetStorage() {
		static std::vector<FactoryType> storage;

		return storage;
	}

#ifndef NDEBUG
	template<typename T>
	class DuplicateCheck {
	public:
		DuplicateCheck() {
			static bool check = false;

			if (check) {
				std::cerr << "State " << typeid(T).name() << " was already registered!" << std::endl;
				std::abort();
			}

			check = true;
		}
	};
#endif // !NDEBUG

	template<typename T, State state>
	static std::nullptr_t Register() {
#ifndef NDEBUG
		static const DuplicateCheck<T> check;
#endif // !NDEBUG

		static constexpr auto index = static_cast<size_t>(state);

		auto& storage = GetStorage();

		if (storage.size() >= index) {
			storage.resize(index + 1);
		}

		assert(!storage[index]);
		assert(storage.size() != 0);

		auto factory = []() -> ReturnType {
			return std::make_unique<T>();
		};

		storage[index] = factory;

		return nullptr;
	}

	static ReturnType Create(State state) {
		const auto index = static_cast<size_t>(state);

		const auto& storage = GetStorage();

		assert(storage.size() >= index);

		return storage[index]();
	}
};

} // namespace utils

template<typename Machine>
class StateMachineBase : public StateMachine {
public:
	using MyState = StateInterface<Machine>;
	using StatePtr = std::unique_ptr<MyState>;
	using StateFactory = utils::FactoryCreator<Machine>;

	template<typename Derived, utils::StateOf<Machine> auto MyState>
	struct StateBase : public MyState {
		static inline auto Registrator = StateFactory::template Register<Derived, MyState>();
	};

protected:
	explicit StateMachineBase(StatePtr&& initial_state) {
		PushState(std::move(initial_state));
	}

	template<utils::StateOf<Machine> MyState>
	explicit StateMachineBase(MyState state) : StateMachineBase(StateFactory::Create(state)) {}

public:
	StatePtr& GetState() {
		assert(!states_.empty());

		return states_.back();
	}

	template<utils::StateOf<Machine> MyState>
	StatePtr& PushState(MyState state) {
		return PushState(StateFactory::Create(state));
	}

	StatePtr& PushState(StatePtr state) {
		return states_.emplace_back(std::move(state));
	}

	StatePtr PopState() {
		auto back = std::move(states_.back());

		states_.pop_back();

		return back;
	}

	void Do(StateValue result) {
		if (result) {
			PushState(result.ToEnum<typename Machine::State>());

			return EnterState();
		} else {
			auto control_value = result.ToControlValue();

			using enum StateValue::ControlValue;

			switch (control_value) {
				case kDoNothing:
				{
					return;
				}
				case kFinish:
				{
					return Finish();
				}
				case kPopState:
				{
					PopState();

					return EnterState();
				}
				case kUnreachable:
				{
					LOG(error) << typeid(Machine).name() << " state " << typeid(*GetState()).name() << " returned unreachable flag!";

					return Finish();
				}
			}
		}
	}

public:
	void EnterState() final {
		StopListeningForInput();

		return Do(GetState()->OnEnter(static_cast<Machine&>(*this)));
	}

	void OnInput(const TgBot::Message::Ptr& input_message) final {
		return Do(GetState()->OnInput(static_cast<Machine&>(*this), input_message));
	}

	void OnCallback(std::string_view data) final {
		return Do(GetState()->OnCallback(static_cast<Machine&>(*this), data));
	}

private:
	std::vector<StatePtr> states_;
};

} // namespace hanley_bot::state