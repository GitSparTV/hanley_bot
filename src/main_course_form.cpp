#include <string>
#include <iostream>

#include <tgbot/types/Message.h>

#include "bot.h"
#include "tg_utils.h"
#include "main_course_form.h"

namespace hanley_bot::dialogs {

using MyStateBase = state::StateBase<MainCourseForm>;
using Value = MyStateBase::Value;
using Self = MyStateBase::Self;
using Message = MyStateBase::Message;

MainCourseForm::MainCourseForm() : StateMachineBase(States::START) {}

Value MainCourseForm::Start::OnEnter(Self self) {
	std::cout << "Согласие на обработку, инфо про набор" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"},
			{tg::utils::ButtonType::kCallback, "Далее >", "continue"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Согласие на обработку, инфо про набор", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::Start::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "continue") {
		return States::INPUT_FIRST_NAME;
	} else if (data == "back") {
		// Call static/groups/get/n
		return true;
	} else {
		return false;
	}
}

Value MainCourseForm::InputFirstName::OnEnter(Self self) {
	std::cout << "Введите имя:" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"},
		}
	};

	self->GetBot().GetAPI().editMessageText("Введите имя:", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	self->ListenForInput();

	return false;
}

Value MainCourseForm::InputFirstName::OnInput([[maybe_unused]] Self self, Message input_message) {
	self->info.name = input_message->text;

	self->GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);

	return States::INPUT_LAST_NAME;
}

Value MainCourseForm::InputFirstName::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "back") {
		return States::START;
	} else {
		return false;
	}
}

Value MainCourseForm::InputLastName::OnEnter([[maybe_unused]] Self self) {
	std::cout << "Введите фамилию:" << std::endl;

	self->ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Введите фамилию:", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::InputLastName::OnInput(Self self, Message input_message) {
	self->info.last_name = input_message->text;

	self->GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);

	return States::INPUT_EMAIL;
}

Value MainCourseForm::InputLastName::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "back") {
		return States::INPUT_FIRST_NAME;
	} else {
		return false;
	}
}

Value MainCourseForm::InputEmail::OnEnter(Self self) {
	std::cout << "Введите почту в FTF аккаунте:" << std::endl;

	self->ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"},
		}
	};

	self->GetBot().GetAPI().editMessageText("Введите почту в FTF аккаунте:", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::InputEmail::OnInput(Self self, Message input_message) {
	if (input_message->text == "invalid") {
		self->ListenForInput();

		tg::utils::MakeKeyboard keyboard{
			{
				{tg::utils::ButtonType::kCallback, "< Назад", "back"},
			}
		};

		self->GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);
		self->GetBot().GetAPI().editMessageText("Введите почту в FTF аккаунте: [Неверный формат]", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

		return false;
	}

	self->GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);
	self->info.email = input_message->text;

	return States::INPUT_WANTS_CREDENTIALING;
}

Value MainCourseForm::InputEmail::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "back") {
		return States::INPUT_LAST_NAME;
	} else {
		return false;
	}
}

Value MainCourseForm::InputWantsCredentialing::OnEnter(Self self) {
	using enum tg::utils::ButtonType;

	std::cout << "Хотите реестр?" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{kCallback, "Да", "yes"},
			{kCallback, "Нет", "no"}
		},
		{
			{kCallback, "< Назад", "back"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Хотите реестр?", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::InputWantsCredentialing::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "yes") {
		self->info.needs_credentialing = true;
	} else if (data == "no") {
		self->info.needs_credentialing = false;
	} else if (data == "back") {
		return States::INPUT_EMAIL;
	} else {
		return false;
	}

	return States::INPUT_PAYMENT_TYPE;
}

Value MainCourseForm::InputPaymentType::OnEnter(Self self) {
	std::cout << "Как будете оплачивать?" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "Рубли", "rubles"}
		},
		{
			{tg::utils::ButtonType::kCallback, "Иностранной картой", "manually"}
		},
		{
			{tg::utils::ButtonType::kCallback, "SWIFT", "swift"}
		},
		{
			{tg::utils::ButtonType::kCallback, "За счёт иностранной организации", "org_invoice"}
		},
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Как будете оплачивать?", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::InputPaymentType::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	using enum PaymentType;

	if (data == "rubles") {
		self->info.payment_type = RUBLES;
	} else if (data == "manually") {
		self->info.payment_type = MANUALLY;
	} else if (data == "swift") {
		self->info.payment_type = SWIFT;
	} else if (data == "org_invoice") {
		self->info.payment_type = ORG_INVOICE;
	} else if (data == "back") {
		return States::INPUT_WANTS_CREDENTIALING;
	} else {
		return false;
	}

	return States::INPUT_NEEDS_TRANSLATION;
}

Value MainCourseForm::InputNeedsTranslation::OnEnter(Self self) {
	std::cout << "Понадобится перевод?" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "Да", "yes"}
		},
		{
			{tg::utils::ButtonType::kCallback, "Нет", "no"}
		},
		{
			{tg::utils::ButtonType::kCallback, "Хочу помочь с переводом", "can_translate"}
		},
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Понадобится перевод?", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::InputNeedsTranslation::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "yes") {
		self->info.needs_translation = NeedsTranslationChoices::YES;
	} else if (data == "no") {
		self->info.needs_translation = NeedsTranslationChoices::NO;
	} else if (data == "can_translate") {
		self->info.needs_translation = NeedsTranslationChoices::CAN_TRANSLATE;

		return States::INPUT_ENGLISH_LEVEL;
	} else if (data == "back") {
		return States::INPUT_PAYMENT_TYPE;
	} else {
		return false;
	}

	return States::CONFIRM;
}

Value MainCourseForm::InputEnglishLevel::OnEnter(Self self) {
	std::cout << "Уровень английского:" << std::endl;

	self->ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Уровень английского:", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::InputEnglishLevel::OnInput(Self self, Message input_message) {
	self->info.english_level = input_message->text;

	self->GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);

	return States::CONFIRM;
}

Value MainCourseForm::InputEnglishLevel::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "back") {
		return States::INPUT_NEEDS_TRANSLATION;
	} else {
		return false;
	}
}

Value MainCourseForm::Confirm::OnEnter(Self self) {
	std::cout << "Проверьте данные" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "Всё верно", "confirm"}
		},
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "back"}
		}
	};

	self->GetBot().GetAPI().editMessageText("Проверьте данные", self->GetMessage()->chat->id, self->GetMessage()->messageId, "", "", false, keyboard);

	return false;
}

Value MainCourseForm::Confirm::OnCallback([[maybe_unused]] Self self, std::string_view data) {
	if (data == "confirm") {
		// Register()
		return true;
	} else if (data == "back") {
		return States::INPUT_NEEDS_TRANSLATION;
	} else {
		return false;
	}
}

} // namespace hanley_bot::dialogs