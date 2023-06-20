#pragma once

#include <cassert>
#include <memory>
#include <string_view>

namespace hanley_bot::state {

class StateValue {
public:
	enum class ControlValue {
		kDoNothing = -1,
		kFinish = -2,
		kPopState = -3,
		kUnreachable = -4
	};

public:
	StateValue() = delete;

	template<typename Enum> requires std::is_enum_v<Enum>
	explicit(false) StateValue(Enum state) : value_(static_cast<int>(state)) {}

	explicit(false) StateValue(ControlValue value) : value_(static_cast<int>(value)) {}

	explicit(false) StateValue(bool value) : StateValue(value ? ControlValue::kFinish : ControlValue::kDoNothing) {}

public:
	inline explicit(false) operator bool() const {
		return value_ >= 0;
	}

	bool operator==(const StateValue&) const = default;

	template<typename Enum> requires std::is_enum_v<Enum>
	inline Enum ToEnum() const {
		assert(value_ >= 0);

		return static_cast<Enum>(value_);
	}

	inline ControlValue ToControlValue() const {
		assert(value_ < 0);

		return static_cast<ControlValue>(value_);
	}

public:
	inline static StateValue DoNothing() {
		return ControlValue::kDoNothing;
	}

	inline static StateValue Finish() {
		return ControlValue::kFinish;
	}

	inline static StateValue PopState() {
		return ControlValue::kPopState;
	}

	inline static StateValue Unreachable() {
		return ControlValue::kUnreachable;
	}

private:
	int value_;
};

template<typename Machine>
class StateBase {
public:
	using Self = Machine&;
	using Message = const TgBot::Message::Ptr&;
	using Value = StateValue;

public:
	virtual ~StateBase() = default;

public:
	virtual Value OnEnter([[maybe_unused]] Self self) { return false; }
	virtual Value OnInput([[maybe_unused]] Self self, [[maybe_unused]] Message input_message) { return false; }
	virtual Value OnCallback([[maybe_unused]] Self self, [[maybe_unused]] std::string_view data) { return false; }
};

} // namespace hanley_bot::state