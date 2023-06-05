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

template<typename Machine, typename Enum>
constexpr void EnumCheck() {
	static_assert(std::is_same_v<typename Machine::States, Enum>, "cannot initialize State with different enum. Must be equal to Machine::States");
}

template<typename Machine>
struct FactoryCreator {
public:
	using ReturnType = typename Machine::StatePtr;
	using EnumType = typename Machine::States;
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

	template<typename T, EnumType enum_index>
	static std::nullptr_t Register() {
#ifndef NDEBUG
		static const DuplicateCheck<T> check;
#endif // !NDEBUG

		static constexpr auto index = static_cast<size_t>(enum_index);

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

	static ReturnType Create(EnumType enum_index) {
		const auto index = static_cast<size_t>(enum_index);

		auto& storage = GetStorage();

		assert(storage.size() >= index);

		return storage[index]();
	}
};

} // namespace utils

template<typename Machine>
class StateMachineBase : public std::enable_shared_from_this<Machine>, public StateMachine {
public:
	using MyStateBase = StateBase<Machine>;
	using StatePtr = std::unique_ptr<MyStateBase>;
	using StateFactory = utils::FactoryCreator<Machine>;

	template<typename Derived, auto Enum> requires std::is_enum_v<decltype(Enum)>
	struct State : public MyStateBase {
		static inline auto Registrator = StateFactory::template Register<Derived, Enum>();

		using Value = MyStateBase::Value;
	};

protected:
	explicit StateMachineBase(StatePtr initial_state) : state_(std::move(initial_state)) {}

	template<typename Enum> requires std::is_enum_v<Enum>
	explicit StateMachineBase(Enum enum_index) : StateMachineBase(StateFactory::Create(enum_index)) {
		utils::EnumCheck<Machine, Enum>();
	}

public:
	StatePtr& GetState() {
		return state_;
	}

	template<typename Enum> requires std::is_enum_v<Enum>
	StatePtr& SetState(Enum enum_index) {
		utils::EnumCheck<Machine, Enum>();

		return SetState(StateFactory::Create(enum_index));
	}

	StatePtr& SetState(StatePtr state) {
		GetState() = std::move(state);

		return GetState();
	}

public:
	void EnterState() final {
		typename MyStateBase::Value result = GetState()->OnEnter(this->shared_from_this());

		if (result) {
			SetState(result.ToEnum());

			EnterState();
		} else {

		}
	}

	void OnInput(const TgBot::Message::Ptr& input_message) final {
		typename MyStateBase::Value result = GetState()->OnInput(this->shared_from_this(), input_message);

		if (result) {
			SetState(result.ToEnum());

			EnterState();
		} else {

		}
	}

	void OnCallback(std::string_view data) final {
		typename MyStateBase::Value result = GetState()->OnCallback(this->shared_from_this(), data);

		if (result) {
			SetState(result.ToEnum());

			EnterState();
		} else {

		}
	}

private:
	StatePtr state_;
};

} // namespace hanley_bot::state