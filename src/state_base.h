#pragma once

#include <cassert>
#include <memory>
#include <string_view>

namespace hanley_bot::state {

template<typename Machine>
class StateBase {
public:
	using Holds = typename Machine::States;
	using Self = std::shared_ptr<Machine>;
	using Message = const TgBot::Message::Ptr&;

public:
	class Value {
	public:
		using Underlying = std::underlying_type_t<Holds>;

		static constexpr Underlying kStateNotChanged = -1;
		static constexpr Underlying kStateFinished = -2;

	public:
		explicit(false) Value(Holds state) : value_(static_cast<Underlying>(state)) {}

		explicit(false) Value(bool value) : value_(value ? kStateFinished : kStateNotChanged) {}

	public:
		explicit(false) operator bool() {
			return value_ >= 0;
		}

		Holds ToEnum() {
			assert(value_ != kStateNotChanged);
			assert(value_ != kStateFinished);

			return static_cast<Holds>(value_);
		}

	private:
		Underlying value_;
	};

public:
	virtual ~StateBase() = default;

public:
	//void SetMachine(std::weak_ptr<Machine> machine) {
	//    assert(!machine.expired());

	//    machine_ = std::move(machine);
	//};

public:
	virtual Value OnEnter([[maybe_unused]] Self self) { return false; }
	virtual Value OnInput([[maybe_unused]] Self self, [[maybe_unused]] Message input_message) { return false; }
	virtual Value OnCallback([[maybe_unused]] Self self, [[maybe_unused]] std::string_view data) { return false; }

	//public:
	//    std::shared_ptr<Machine> GetMachine() {
	//        assert(!machine_.expired());
	//
	//        return machine_.lock();
	//    }
	//
	//private:
	//    std::weak_ptr<Machine> machine_;
};

} // namespace hanley_bot::state