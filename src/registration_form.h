#pragma once

#include <string>

#include "state_machine_base.h"

namespace hanley_bot::dialogs {

class RegistrationForm final : public state::StateMachineBase<RegistrationForm> {
public:
	enum class State {
		START,
		INPUT_FIRST_NAME,
		INPUT_LAST_NAME,
		INPUT_EMAIL,
		INPUT_NEEDS_TRANSLATION,
		INPUT_ENGLISH_LEVEL,
		CONFIRM,
		FINISH
	};

public:
	RegistrationForm();

public:
	void ReceiveArgs(const std::string& called_from) {
		called_from_ = called_from;
	}

	void Register();

private:
	enum class NeedsTranslationChoices {
		YES,
		NO,
		CAN_TRANSLATE
	};

	struct RegistrationInfo {
		std::string name;
		std::string last_name;
		std::string email;
		NeedsTranslationChoices needs_translation = NeedsTranslationChoices::YES;
		std::string english_level;
	};

private:
	struct Start final : public StateBase<Start, State::START> {
		Value OnEnter(Self self) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct InputFirstName final : public StateBase<InputFirstName, State::INPUT_FIRST_NAME> {
		Value OnEnter(Self self) override;

		Value OnInput(Self self, Message input_message) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct InputLastName final : public StateBase<InputLastName, State::INPUT_LAST_NAME> {
		Value OnEnter(Self self) override;

		Value OnInput(Self self, Message input_message) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct InputEmail final : public StateBase<InputEmail, State::INPUT_EMAIL> {
		Value OnEnter(Self self) override;

		Value OnInput(Self self, Message input_message) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct InputNeedsTranslation final : public StateBase<InputNeedsTranslation, State::INPUT_NEEDS_TRANSLATION> {
		Value OnEnter(Self self) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct InputEnglishLevel final : public StateBase<InputEnglishLevel, State::INPUT_ENGLISH_LEVEL> {
		Value OnEnter(Self self) override;

		Value OnInput(Self self, Message input_message) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct Confirm final : public StateBase<Confirm, State::CONFIRM> {
		Value OnEnter(Self self) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct Finish final : public StateBase<Finish, State::FINISH> {
		Value OnEnter(Self self) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

private:
	std::string called_from_;
	RegistrationInfo info_;
};

} // namespace hanley_bot::dialogs