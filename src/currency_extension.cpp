#include "sdk.h"

#include <fmt/core.h>
#include <pqxx/pqxx>
#include <boost/json.hpp>
#include <tgbot/net/BoostHttpOnlySslClient.h>

#include "currency_extension.h"
#include "logging.h"

namespace hanley_bot::currency {

double FetchConversionRate(std::string_view token) {
	TgBot::BoostHttpOnlySslClient client;

	const auto result = client.makeRequest(
		fmt::format("https://openexchangerates.org/api/latest.json?app_id={}&symbols=RUB", token), {});

	const auto result_json = boost::json::parse(result);

	const auto& table = result_json.as_object();

	if (table.contains("error")) {
		LOG_VERBOSE(error) << "Failed to get currency. Response: " << result;

		return 0;
	}

	double rate = table.at("rates").at("RUB").as_double();

	LOG_VERBOSE(debug) << fmt::format("Got {} currency rate", rate);

	return rate;
}

} // namespace hanley_bot::currency