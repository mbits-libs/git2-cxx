// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <git2/global.hh>
#include <args/parser.hpp>

extern int tool(args::args_view const&);

int main(int argc, char* argv[]) {
	git::init memory_suite{};
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	return tool(args::from_main(argc, argv));
}
