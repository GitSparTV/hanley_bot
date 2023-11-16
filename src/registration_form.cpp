#include <string>
#include <iostream>

#include <fmt/core.h>
#include <tgbot/types/Message.h>

#include "bot.h"
#include "tg_utils.h"
#include "registration_form.h"
#include "bot_commands.h"

namespace hanley_bot::dialogs {

RegistrationForm::RegistrationForm() : StateMachineBase(State::START) {}

auto RegistrationForm::Start::OnEnter(Self self) -> Value {
	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "❌ Отмена", "d_exit"},
			{tg::utils::ButtonType::kCallback, "Далее ➡️", "d_continue"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Согласие на обработку, инфо про набор", keyboard);

	return false;
}

auto RegistrationForm::Start::OnCallback(Self self, std::string_view data) -> Value {
	if (data == "d_continue") {
		return State::INPUT_FIRST_NAME;
	} else if (data == "d_exit") {
		self.GetBot().DeleteMessage(self.GetContext());

		return true;
	} else {
		return Value::Unreachable();
	}
}

auto RegistrationForm::InputFirstName::OnEnter(Self self) -> Value {
	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"},
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Введите имя:", keyboard);

	self.ListenForInput();

	return false;
}

auto RegistrationForm::InputFirstName::OnInput(Self self, Message input_message) -> Value {
	self.info_.name = input_message->text;

	return State::INPUT_LAST_NAME;
}

auto RegistrationForm::InputFirstName::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return Value::Unreachable();
	}
}

auto RegistrationForm::InputLastName::OnEnter(Self self) -> Value {
	self.ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Введите фамилию:", keyboard);

	return false;
}

auto RegistrationForm::InputLastName::OnInput(Self self, Message input_message) -> Value {
	self.info_.last_name = input_message->text;

	return State::INPUT_EMAIL;
}

auto RegistrationForm::InputLastName::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return Value::Unreachable();
	}
}

auto RegistrationForm::InputEmail::OnEnter(Self self) -> Value {
	self.ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"},
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Введите почту в FTF аккаунте:", keyboard);

	return false;
}

auto RegistrationForm::InputEmail::OnInput(Self self, Message input_message) -> Value {
	if (input_message->text == "invalid") {
		self.ListenForInput();

		tg::utils::MakeKeyboard keyboard{
			{
				{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"},
			}
		};

		self.GetBot().EditMessage(self.GetContext(), "Введите почту в FTF аккаунте: [Неверный формат]", keyboard);

		return false;
	}

	self.info_.email = input_message->text;

	return State::INPUT_NEEDS_TRANSLATION;
}

auto RegistrationForm::InputEmail::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return Value::Unreachable();
	}
}

auto RegistrationForm::InputNeedsTranslation::OnEnter(Self self) -> Value {
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
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Понадобится перевод?", keyboard);

	return false;
}

auto RegistrationForm::InputNeedsTranslation::OnCallback(Self self, std::string_view data) -> Value {
	if (data == "d_yes") {
		self.info_.needs_translation = NeedsTranslationChoices::YES;
	} else if (data == "d_no") {
		self.info_.needs_translation = NeedsTranslationChoices::NO;
	} else if (data == "d_can_translate") {
		self.info_.needs_translation = NeedsTranslationChoices::CAN_TRANSLATE;

		return State::INPUT_ENGLISH_LEVEL;
	} else if (data == "d_back") {
		return Value::PopState();
	} else {
		return Value::Unreachable();
	}

	return State::CONFIRM;
}

auto RegistrationForm::InputEnglishLevel::OnEnter(Self self) -> Value {
	self.ListenForInput();

	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"}
		}
	};

	self.GetBot().EditMessage(self.GetContext(), "Уровень английского:", keyboard);

	return false;
}

auto RegistrationForm::InputEnglishLevel::OnInput(Self self, Message input_message) -> Value {
	self.info_.english_level = input_message->text;

	return State::CONFIRM;
}

auto RegistrationForm::InputEnglishLevel::OnCallback([[maybe_unused]] Self self, std::string_view data) -> Value {
	if (data == "d_back") {
		return Value::PopState();
	} else {
		return Value::Unreachable();
	}
}

auto RegistrationForm::Confirm::OnEnter(Self self) -> Value {
	tg::utils::MakeKeyboard keyboard{
		{
			{tg::utils::ButtonType::kCallback, "👍 Всё верно", "d_confirm"}
		},
		{
			{tg::utils::ButtonType::kCallback, "⬅️ Назад", "d_back"}
		}
	};

	const auto& info = self.info_;
	const auto& name = info.name;
	const auto& last_name = info.last_name;
	const auto& email = info.email;
	const auto& needs_translation = info.needs_translation;
	const auto& english_level = info.english_level;

	std::string_view needs_translation_serialized;
	std::string english_level_serialized;

	switch (needs_translation) {
		case NeedsTranslationChoices::YES:
		{
			needs_translation_serialized = "Да";

			break;
		}
		case NeedsTranslationChoices::NO:
		{
			needs_translation_serialized = "Нет";

			break;
		}
		case NeedsTranslationChoices::CAN_TRANSLATE:
		{
			needs_translation_serialized = "Нет, могу помочь с переводом";
			english_level_serialized = fmt::format("\n<b>Уровень английского:</b> {}", english_level);

			break;
		}
	}

	const auto message = fmt::format("<b>Проверьте данные:</b>\n\n<b>Имя:</b> {}\n<b>Фамилия:</b> {}\n<b>Почта на сайте FTF:</b> {}\n<b>Нужен ли перевод:</b> {}{}",
		name, last_name, email, needs_translation_serialized, english_level_serialized);

	self.GetBot().EditMessage(self.GetContext(), message, keyboard, "HTML");

	return false;
}

void RegistrationForm::Register() {
	auto& tx = GetBot().BeginTransaction();

	const auto& name = info_.name;
	const auto& last_name = info_.last_name;
	const auto& email = info_.email;
	const auto& needs_translation = info_.needs_translation;
	const auto& english_level = info_.english_level;

	tx.exec0(fmt::format("INSERT INTO students (telegram_id, name, surname, email, needs_translation, english_level) VALUES ({}, '{}', '{}', '{}', {}, NULLIF('{}', '')) ON CONFLICT DO NOTHING",
		GetContext().user, tx.esc(name), tx.esc(last_name), tx.esc(email),
		needs_translation == NeedsTranslationChoices::YES ? "TRUE" : "FALSE",
		tx.esc(english_level)
	));

	GetBot().Commit();
}

auto RegistrationForm::Confirm::OnCallback(Self self, std::string_view data) -> Value {
	if (data == "d_confirm") {
		self.Register();

		return State::FINISH;
	} else if (data == "d_back") {
		return Value::PopState();
	} else {
		return Value::Unreachable();
	}
}

auto RegistrationForm::Finish::OnEnter(Self self) -> Value {
	tg::utils::KeyboardBuilder keyboard;

	if (!self.called_from_.empty()) {
		keyboard.Row().Callback("➡️ Записаться на обучение", "d_continue");
	}

	keyboard.Row().Callback("🏁 Закрыть", "d_close");

	self.GetBot().EditMessage(self.GetContext(), "🎉 Вы успешно зарегистрированы!\nБазовая информация о вас больше не потребуется перед записью на обучение.", keyboard);

	return false;
}

auto RegistrationForm::Finish::OnCallback(Self self, std::string_view data) -> Value {
	if (data == "d_continue" && !self.called_from_.empty()) {
		hanley_bot::commands::CallCommand(self.GetBot(), fmt::format("static_groups_reg_{}", self.called_from_), self.GetContext());
	} else if (data == "d_close") {
		self.GetBot().DeleteMessage(self.GetContext());
	} else {
		return Value::Unreachable();
	}

	return true;
}

} // namespace hanley_bot::dialogs