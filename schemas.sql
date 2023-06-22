-- Кэш курса доллара к рублю. Не храним как MONEY потому что не так важна точность в этом контексте
CREATE TABLE exchange_cache (
	time TIMESTAMPTZ NOT NULL DEFAULT NOW(), -- Время записи кэша
	rate DOUBLE PRECISION NOT NULL -- рублей в 1 долларе
);

-- Проверяем кэш на валидность по времени, возвращаем найденный кэш, если есть
DELETE FROM exchange_cache WHERE time < NOW() - INTERVAL '1 day';
SELECT rate FROM exchange_cache WHERE time > NOW() - INTERVAL '1 day';

-- Запись нового числа
INSERT INTO exchange_cache (rate) VALUES ({});

----

-- Сброс id после отладки
TRUNCATE TABLE users RESTART IDENTITY;

-- Таблица всех пользователей, кто инициировал диалог с ботом. Используется для статистики и глобальных новостей.
CREATE TABLE users (
	id INT GENERATED ALWAYS AS IDENTITY, -- Счётчик уникальных пользователей
	telegram_id BIGINT PRIMARY KEY, -- Телеграм ID человека
	time TIMESTAMPTZ NOT NULL DEFAULT NOW(), -- Дата первого контакта для статистики
	first_name TEXT NOT NULL, -- Имя
	last_name TEXT, -- Фамилия
);

-- При инициировании диалога будет вызвано это выражение
SELECT EXISTS(SELECT 1 FROM users WHERE telegram_id = {}) -- Проверяем, что ещё не зарегистрирован

INSERT INTO users (telegram_id, first_name, last_name) VALUES ({}, '{}', NULLIF('{}','')) -- Регистрируем

-- База курсов с описанием на русском
CREATE TABLE courses (
	id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
	full_name TEXT UNIQUE NOT NULL, -- Полное название курса
	short_name VARCHAR(10) UNIQUE NOT NULL, -- Суффикс для название телеграм группы
	url TEXT UNIQUE NOT NULL, -- Ссылка на сайте
	description TEXT NOT NULL
);

-- Добавление курса в базу
INSERT INTO courses (full_name, short_name, url, description) VALUES ('', '', '', '');

-- Получить список курсов, дополнительно третей колонкой вывести инфу подписан ли данный человек на новости об этом курсе или нет
SELECT id, full_name, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed
FROM courses
LEFT JOIN subscriptions
ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {}
ORDER BY courses.id ASC

-- Получить описание конкретного курса
SELECT full_name, description, url, CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed
FROM courses
LEFT JOIN subscriptions
ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {}
WHERE id = {}

-- При подписке на курс
SELECT full_name FROM courses WHERE id = {} -- Берём название курса для вывода
INSERT INTO subscriptions (telegram_id, course_id) VALUES ({}, {}) ON CONFLICT DO NOTHING -- Если подписать
DELETE FROM subscriptions WHERE telegram_id = {} AND course_id = {} -- Если отписать

---

-- Пока нет группы/потока/когорты, люди могут подписаться на уведомления.
CREATE TABLE subscriptions (
	telegram_id BIGINT, -- Телеграм ID человека
	course_id INT NOT NULL REFERENCES courses(id) ON DELETE CASCADE, -- ID интересующего курса
	time_added TIMESTAMP NOT NULL DEFAULT NOW(), -- Для статистики
	PRIMARY KEY (telegram_id, course_id) -- Делаем композитный ключ, чтобы ограничить попадание двойной подписки у человека и для быстрого поиска как и по подпискам пользователя, так и по подпичсикам группы
);

-- Добавить
INSERT INTO subscriptions (telegram_id, course_id) VALUES ({}, {}) ON CONFLICT DO NOTHING
-- Удалить
DELETE FROM subscriptions WHERE telegram_id = {} AND course_id = {}

-- Получение списка курсов и статуса подписки на него
SELECT id, full_name CASE WHEN subscriptions.course_id IS NOT NULL THEN true ELSE false END AS subscribed FROM courses ORDER BY id ASC LEFT JOIN subscriptions ON courses.id = subscriptions.course_id AND subscriptions.telegram_id = {}

-- Статистика сколько людей на какой курс подписались
SELECT c.short_name, COUNT(s.telegram_id) FROM courses AS c LEFT JOIN subscriptions AS s ON c.id = s.course_id GROUP BY c.id, c.short_name ORDER BY c.id ASC

---

-- Товары, которые можно приобрести внутри курса. По сути несколько продуктов будет только у сертификации и где требуется компенсация
CREATE TABLE products (
	id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
	course_id INT NOT NULL REFERENCES courses(id) ON DELETE CASCADE, -- Курс, к которому относится товар
	product_name TEXT UNIQUE NOT NULL, -- Наименование покупки
	price MONEY NOT NULL -- Цена в долларах. Конверсия в рубли устанавливается в группе отдельно
);

---

-- Учащийся на потоке. В целом, нам требуется один раз зарегистрировать, а дополнительную информацию спрашивать уже в отдельной форме
CREATE TABLE students (
	telegram_id BIGINT PRIMARY KEY, -- Телеграм ID человека
	name TEXT NOT NULL, -- Имя
	surname TEXT NOT NULL, -- Фамилия
	email TEXT UNIQUE NOT NULL, -- Почта, через которую заходит на сайт, очень важно
	needs_translation BOOLEAN NOT NULL, -- Нужен ли человеку перевод
	english_level TEXT, -- Уровень английского, если вдруг захочет когда-то помочь перевести
	register_date TIMESTAMP NOT NULL DEFAULT NOW(), -- Дата регистрации, для статистики
	UNIQUE (telegram_id, email) -- Не может быть использована та же почта у разных людей
);

---

-- Лимиты групп. К примеру, для нескольких групп есть общий бюджет и за ним нужно проследить, чтобы не выйти из него.
-- Для других групп может быть актуален лимит по количеству людей.
CREATE TABLE groups_limits (
	id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, -- ID лимита
	name TEXT UNIQUE NOT NULL, -- Название лимита
	seats_limit INT, -- Количество доступных мест
	seats_taken INT, -- Количество занятых мест
	money_limit MONEY, -- Бюджет лимита
	money_total MONEY, -- Текущее количество денег
	CHECK (seats_taken <= seats_limit), -- Нельзя зайти, если лимит людей превышен
	CHECK (money_total <= money_limit) -- Нельзя зайти, если лимит бюджета превышен
);

---

-- Поток/когорта на обучение
CREATE TABLE groups (
	id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, -- Уникальный ID группы
	course_id INT NOT NULL REFERENCES courses(id), -- ID курса по которому делается поток
	cohort INT UNIQUE NOT NULL, -- Номер потока
	chat_id BIGINT UNIQUE NOT NULL, -- ID телеграм-чата. Бот будет проверять, что в группе есть только разрешённые пользователи
	invite_link TEXT UNIQUE NOT NULL, -- Инвайт-ссылка в телеграм потока
	is_open BOOLEAN NOT NULL, -- Открыта ли ещё группа?
	limit_constraint INT REFERENCES groups_limits(id), -- ID лимита группы
	UNIQUE (course_id, cohort) -- У группы может быть только один такой номер потока
);

---

-- Информация о студенте в потоке
CREATE TABLE groups_users_info (
	group_id INT NOT NULL REFERENCES groups(id), -- ID потока
	telegram_id INT NOT NULL REFERENCES students(telegram_id), -- Телеграм ID человека
	product_id INT NOT NULL REFERENCES products(id), -- ID продукта
	last_ping TIMESTAMP NOT NULL, -- Дата последнего опроса, что человеку ещё актуально и он заходит в телеграм
	payment_method INT NOT NULL, -- Тип оплаты. Enum
	payment_status INT, -- Статус оплаты
	role TEXT, -- Роль в группе. Например, переводчик или посредник
	register_date TIMESTAMP NOT NULL, -- Дата регистрации, для статистики
	PRIMARY KEY (group_id, telegram_id) -- Человек может в той же группе только один
);