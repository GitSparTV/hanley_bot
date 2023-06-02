cmake_minimum_required(VERSION 3.22)

project(hanley_bot CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

add_executable(hanley_bot
	src/main.cpp
	src/tg_utils.h
	src/tg_utils.cpp
	src/tg_debug.h
	src/tg_debug.cpp
	src/bot.h
	src/bot.cpp
	src/state_machine.cpp
	src/state_machine.h
	src/state_base.h
	src/state_machine_base.cpp
	src/state_machine_base.h
	src/states_controller.cpp
	src/states_controller.h
	src/main_course_form.cpp
	src/main_course_form.h
	)
target_include_directories(hanley_bot PRIVATE CONAN_PKG::tgbot CONAN_PKG::boost CONAN_PKG::libpqxx)
target_link_libraries(hanley_bot PRIVATE CONAN_PKG::tgbot CONAN_PKG::boost CONAN_PKG::libpqxx)