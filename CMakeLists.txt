cmake_minimum_required(VERSION 3.22)

project(hanley_bot CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)
add_compile_definitions(_SILENCE_CXX20_CISO646_REMOVED_WARNING)

add_executable(hanley_bot
	src/main.cpp
	src/sdk.h
	src/logging.cpp
	src/logging.h
	src/domain.cpp
	src/domain.h
	src/tg_utils.cpp
	src/tg_utils.h
	src/tg_debug.cpp
	src/tg_debug.h
	src/bot.cpp
	src/bot.h
	src/bot_commands.cpp
	src/bot_commands.h
	src/config.h
	src/boost_json.cpp
	src/config_json_reader.cpp
	src/config_json_reader.h
	src/state_machine.cpp
	src/state_machine.h
	src/state_base.h
	src/state_machine_base.h
	src/states_controller.cpp
	src/states_controller.h
	src/main_course_form.cpp
	src/main_course_form.h
	src/currency_extension.cpp
	src/currency_extension.h
	)

target_compile_options(hanley_bot PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_include_directories(hanley_bot PRIVATE CONAN_PKG::tgbot CONAN_PKG::boost CONAN_PKG::libpqxx CONAN_PKG::fmt)
target_link_libraries(hanley_bot PRIVATE CONAN_PKG::tgbot CONAN_PKG::boost CONAN_PKG::libpqxx CONAN_PKG::fmt)