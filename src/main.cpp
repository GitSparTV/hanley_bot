#include "sdk.h"
#include "logging.h"
#include "bot.h"
#include "bot_commands.h"
#include "config_json_reader.h"

int main(int argc, char* argv[]) {
	hanley_bot::logger::InitConsole();
	hanley_bot::logger::HookSignals();

	std::vector<std::string_view> args(argv + 1, argv + argc);

	if (argc < 2) {
		LOG(error) << "./hanley_bot <config_file> [<push_to_telegram>]" << std::endl;

		return EXIT_FAILURE;
	}

	auto config = hanley_bot::config::FromJSONFile(args[0]);

	hanley_bot::logger::InitFile(config.log_folder);

	try {
		hanley_bot::Bot bot(config);

		if (args.size() >= 2 && args[1] == "push") {
			LOG(info) << "Pushing commands to Telegram";

			hanley_bot::commands::PushCommands(bot);
		}

		hanley_bot::commands::InitializeCommands(bot);

		bot.Run();
	} catch (const std::exception& ex) {
		LOG(fatal) << "Exception caught in main (" << typeid(ex).name() << "): " << ex.what();

		return EXIT_FAILURE;
	} catch (...) {
		LOG(fatal) << "Unknown exception caught in main";

		return EXIT_FAILURE;
	}

	LOG(warning) << "bot.Run() finished?";

	return EXIT_SUCCESS;
}