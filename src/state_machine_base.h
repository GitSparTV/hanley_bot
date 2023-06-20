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
concept StatesOf = std::is_same_v<typename Machine::States, Enum>;

template<typename Machine>
struct FactoryCreator {
public:
	using ReturnType = typename Machine::StatePtr;
	using States = typename Machine::States;
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

	template<typename T, States state>
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

	static ReturnType Create(States state) {
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
	using MyStateBase = StateBase<Machine>;
	using StatePtr = std::unique_ptr<MyStateBase>;
	using StateFactory = utils::FactoryCreator<Machine>;

	template<typename Derived, utils::StatesOf<Machine> auto MyStates>
	struct State : public MyStateBase {
		static inline auto Registrator = StateFactory::template Register<Derived, MyStates>();
	};

protected:
	explicit StateMachineBase(StatePtr&& initial_state) {
		PushState(std::move(initial_state));
	}

	template<utils::StatesOf<Machine> MyStates>
	explicit StateMachineBase(MyStates state) : StateMachineBase(StateFactory::Create(state)) {}

public:
	StatePtr& GetState() {
		assert(!states_.empty());

		return states_.back();
	}

	template<utils::StatesOf<Machine> MyStates>
	StatePtr& PushState(MyStates state) {
		return PushState(StateFactory::Create(state));
	}

	StatePtr& PushState(StatePtr state) {
		return states_.emplace_back(std::move(state));
	}

	StatePtr PopState() {
		auto back = states_.back();

		states_.pop_back();

		return back;
	}

public:
	void EnterState() final {
		StateValue result = GetState()->OnEnter(static_cast<Machine&>(*this));

		if (result) {
			PushState(result.ToEnum<typename Machine::States>());

			EnterState();
		} else {

		}
	}

	void OnInput(const TgBot::Message::Ptr& input_message) final {
		StateValue result = GetState()->OnInput(static_cast<Machine&>(*this), input_message);

		if (result) {
			PushState(result.ToEnum<typename Machine::States>());

			EnterState();
		} else {

		}
	}

	void OnCallback(std::string_view data) final {
		StateValue result = GetState()->OnCallback(static_cast<Machine&>(*this), data);

		if (result) {
			PushState(result.ToEnum<typename Machine::States>());

			EnterState();
		} else {

		}
	}

private:
	std::vector<StatePtr> states_;
};

} // namespace hanley_bot::state