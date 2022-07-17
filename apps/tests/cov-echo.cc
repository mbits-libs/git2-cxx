// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>

using namespace std::literals;

static inline void print_one(std::string_view arg) {
	bool has_apos = arg.find('\'') != std::string::npos;
	bool needs_escape = false;
	for (auto esc : R"(\ ")"sv) {
		needs_escape = arg.find(esc) != std::string::npos;
		if (needs_escape) break;
	}

	if (!has_apos && !needs_escape) {
		fmt::print("{}", arg);
		return;
	}

	if (!has_apos) {
		fmt::print("'{}'", arg);
		return;
	}

	std::fputc('"', stdout);
	for (auto c : arg) {
		switch (c) {
			case '\\':
			case '"':
				std::fputc('\\', stdout);
				break;
		}
		std::fputc(c, stdout);
	}
	std::fputc('"', stdout);
}

int tool(args::args_view const& arguments) {

	print_one(arguments.progname);

	for (unsigned index = 0; index < arguments.args.size(); ++index) {
		auto const arg = arguments.args[index];
		std::fputc(' ', stdout);
		print_one(arg);
	}
	std::fputc('\n', stdout);
	return 0;
}
