#include <iostream>

#include "logging.h"
#include "bot.h"
#include "bot_commands.h"
#include "config_json_reader.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	std::vector<std::string_view> args(argv + 1, argv + argc);

	if (argc < 2) {
		std::cout << "./hanley_bot <config_file> [<push_to_telegram>]" << std::endl;

		return EXIT_FAILURE;
	}

	hanley_bot::logger::Init();

	auto config = hanley_bot::config::FromJSONFile(args[0]);

	try {
		hanley_bot::Bot bot(config);

		if (args.size() >= 2 && args[1] == "push") {
			LOG(info) << "Pushing commands to Telegram";

			hanley_bot::commands::PushCommands(bot);
		}

		hanley_bot::commands::InitializeCommands(bot);

		bot.Run();
	} catch (const std::exception& ex) {
		std::cout << "Exception caught in main (" << typeid(ex).name() << "): " << ex.what() << std::endl;
		return EXIT_FAILURE;
	} catch (...) {
		std::cout << "Unknown exception caught in main" << std::endl;
	}

	return EXIT_SUCCESS;
}