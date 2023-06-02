#pragma once

#include <string>

#include "state_machine_base.h"

namespace hanley_bot::dialogs {

class MainCourseForm final : public state::StateMachineBase<MainCourseForm> {
public:
    enum class States {
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
    struct Start final : public State<Start, States::START> {
        Value OnEnter(Self self) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputFirstName final : public State<InputFirstName, States::INPUT_FIRST_NAME> {
        Value OnEnter(Self self) override;

        Value OnInput(Self self, Message input_message) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputLastName final : public State<InputLastName, States::INPUT_LAST_NAME> {
        Value OnEnter(Self self) override;

        Value OnInput(Self self, Message input_message) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputEmail final : public State<InputEmail, States::INPUT_EMAIL> {
        Value OnEnter(Self self) override;

        Value OnInput(Self self, Message input_message) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputWantsCredentialing final : public State<InputWantsCredentialing, States::INPUT_WANTS_CREDENTIALING> {
        Value OnEnter(Self self) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputPaymentType final : public State<InputPaymentType, States::INPUT_PAYMENT_TYPE> {
        Value OnEnter(Self self) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputNeedsTranslation final : public State<InputNeedsTranslation, States::INPUT_NEEDS_TRANSLATION> {
        Value OnEnter(Self self) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct InputEnglishLevel final : public State<InputEnglishLevel, States::INPUT_ENGLISH_LEVEL> {
        Value OnEnter(Self self) override;

        Value OnInput(Self self, Message input_message) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

    struct Confirm final : public State<Confirm, States::CONFIRM> {
        Value OnEnter(Self self) override;

        Value OnCallback(Self self, std::string_view data) override;
    };

private:
    RegistrationInfo info;
};

} // namespace hanley_bot::dialogs