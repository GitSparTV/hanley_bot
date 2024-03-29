#pragma once

#include <string>

#include "state_machine_base.h"

namespace hanley_bot::dialogs {

class MainCourseForm final : public state::StateMachineBase<MainCourseForm> {
public:
	enum class State {
		START,
		INPUT_FIRST_NAME,
		INPUT_LAST_NAME,
		INPUT_EMAIL,
		INPUT_WANTS_CREDENTIALING,
		INPUT_PAYMENT_TYPE,
		INPUT_NEEDS_TRANSLATION,
		INPUT_ENGLISH_LEVEL,
		CONFIRM
	};

public:
	MainCourseForm();

private:
	enum class PaymentType {
		RUBLES,
		MANUALLY,
		SWIFT,
		ORG_INVOICE
	};

	enum class NeedsTranslationChoices {
		YES,
		NO,
		CAN_TRANSLATE
	};

	struct RegistrationInfo {
		std::string name;
		std::string last_name;
		std::string email;
		bool needs_credentialing = false;
		PaymentType payment_type = PaymentType::RUBLES;
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

	struct InputWantsCredentialing final : public StateBase<InputWantsCredentialing, State::INPUT_WANTS_CREDENTIALING> {
		Value OnEnter(Self self) override;

		Value OnCallback(Self self, std::string_view data) override;
	};

	struct InputPaymentType final : public StateBase<InputPaymentType, State::INPUT_PAYMENT_TYPE> {
		Value OnEnter(Self self) override;

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

private:
	RegistrationInfo info;
};

} // namespace hanley_bot::dialogs