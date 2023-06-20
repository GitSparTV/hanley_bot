#include <string>
#include <iostream>

#include <tgbot/types/Message.h>

#include "bot.h"
#include "tg_utils.h"
#include "main_course_form.h"

namespace hanley_bot::dialogs {

MainCourseForm::MainCourseForm() : StateMachineBase(States::START) {}

auto MainCourseForm::Start::OnEnter(Self self) -> Value {
	std::cout << "Согласие на обработку, инфо про набор" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"},
			{tg::utils::ButtonType::kCallback, "Далее >", "d_continue"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Согласие на обработку, инфо про набор", keyboard);

	return false;
}

auto MainCourseForm::Start::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_continue") {
		return States::INPUT_FIRST_NAME;
	} else if (data == "d_back") {
		// Call static/groups/get/n
		return true;
	} else {
		return false;
	}
}

auto MainCourseForm::InputFirstName::OnEnter(Self self) -> Value {
	std::cout << "Введите имя:" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"},
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Введите имя:", keyboard);

	self.ListenForInput();

	return false;
}

auto MainCourseForm::InputFirstName::OnInput([[maybe_unused]] Self self, Message input_message) -> Value {
	self.info.name = input_message->text;

	self.GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);

	return States::INPUT_LAST_NAME;
}

auto MainCourseForm::InputFirstName::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}
}

auto MainCourseForm::InputLastName::OnEnter([[maybe_unused]] Self self) -> Value {
	std::cout << "Введите фамилию:" << std::endl;

	self.ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Введите фамилию:", keyboard);

	return false;
}

auto MainCourseForm::InputLastName::OnInput(Self self, Message input_message) -> Value {
	self.info.last_name = input_message->text;

	self.GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);

	return States::INPUT_EMAIL;
}

auto MainCourseForm::InputLastName::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}
}

auto MainCourseForm::InputEmail::OnEnter(Self self) -> Value {
	std::cout << "Введите почту в FTF аккаунте:" << std::endl;

	self.ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"},
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Введите почту в FTF аккаунте:", keyboard);

	return false;
}

auto MainCourseForm::InputEmail::OnInput(Self self, Message input_message) -> Value {
	if (input_message->text == "invalid") {
		self.ListenForInput();

		tg::utils::MakeKeyboard keyboard{
			{
				{tg::utils::ButtonType::kCallback, "< Назад", "d_back"},
			}
		};

		self.GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);
		self.GetBot().EditMessage(self.GetContext(), "Введите почту в FTF аккаунте: [Неверный формат]", keyboard);

		return false;
	}

	self.GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);
	self.info.email = input_message->text;

	return States::INPUT_WANTS_CREDENTIALING;
}

auto MainCourseForm::InputEmail::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}
}

auto MainCourseForm::InputWantsCredentialing::OnEnter(Self self) -> Value {
	using enum tg::utils::ButtonType;

	std::cout << "Хотите реестр?" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{kCallback, "Да", "d_yes"},
			{kCallback, "Нет", "d_no"}
		},
		{
			{kCallback, "< Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Хотите реестр?", keyboard);

	return false;
}

auto MainCourseForm::InputWantsCredentialing::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_yes") {
		self.info.needs_credentialing = true;
	} else if (data == "d_no") {
		self.info.needs_credentialing = false;
	} else if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}

	return States::INPUT_PAYMENT_TYPE;
}

auto MainCourseForm::InputPaymentType::OnEnter(Self self) -> Value {
	std::cout << "Как будете оплачивать?" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "Рубли", "d_rubles"}
		},
		{
			{tg::utils::ButtonType::kCallback, "Иностранной картой", "d_manually"}
		},
		{
			{tg::utils::ButtonType::kCallback, "SWIFT", "d_swift"}
		},
		{
			{tg::utils::ButtonType::kCallback, "За счёт иностранной организации", "d_org_invoice"}
		},
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Как будете оплачивать?", keyboard);

	return false;
}

auto MainCourseForm::InputPaymentType::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	using enum PaymentType;

	if (data == "d_rubles") {
		self.info.payment_type = RUBLES;
	} else if (data == "d_manually") {
		self.info.payment_type = MANUALLY;
	} else if (data == "d_swift") {
		self.info.payment_type = SWIFT;
	} else if (data == "d_org_invoice") {
		self.info.payment_type = ORG_INVOICE;
	} else if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}

	return States::INPUT_NEEDS_TRANSLATION;
}

auto MainCourseForm::InputNeedsTranslation::OnEnter(Self self) -> Value {
	std::cout << "Понадобится перевод?" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "Да", "d_yes"}
		},
		{
			{tg::utils::ButtonType::kCallback, "Нет", "d_no"}
		},
		{
			{tg::utils::ButtonType::kCallback, "Хочу помочь с переводом", "d_can_translate"}
		},
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Понадобится перевод?", keyboard);

	return false;
}

auto MainCourseForm::InputNeedsTranslation::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_yes") {
		self.info.needs_translation = NeedsTranslationChoices::YES;
	} else if (data == "d_no") {
		self.info.needs_translation = NeedsTranslationChoices::NO;
	} else if (data == "d_can_translate") {
		self.info.needs_translation = NeedsTranslationChoices::CAN_TRANSLATE;

		return States::INPUT_ENGLISH_LEVEL;
	} else if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}

	return States::CONFIRM;
}

auto MainCourseForm::InputEnglishLevel::OnEnter(Self self) -> Value {
	std::cout << "Уровень английского:" << std::endl;

	self.ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Уровень английского:", keyboard);

	return false;
}

auto MainCourseForm::InputEnglishLevel::OnInput(Self self, Message input_message) -> Value {
	self.info.english_level = input_message->text;

	self.GetBot().GetAPI().deleteMessage(input_message->chat->id, input_message->messageId);

	return States::CONFIRM;
}

auto MainCourseForm::InputEnglishLevel::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}
}

auto MainCourseForm::Confirm::OnEnter(Self self) -> Value {
	std::cout << "Проверьте данные" << std::endl;

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "Всё верно", "d_confirm"}
		},
		{
			{tg::utils::ButtonType::kCallback, "< Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Проверьте данные", keyboard);

	return false;
}

auto MainCourseForm::Confirm::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_confirm") {
		// Register()
		return true;
	} else if (data == "d_back") {
		return Value::PopState();
	} else {
		return false;
	}
}

} // namespace hanley_bot::dialogs