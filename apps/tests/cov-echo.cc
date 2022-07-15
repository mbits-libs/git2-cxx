// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>

int main(int argc, char** argv) {
	using namespace std::literals;

	auto args = args::arglist{argc, argv};
	bool first = true;
	for (unsigned index = 0; index < args.size(); ++index) {
		if (first)
			first = false;
		else
			std::fputc(' ', stdout);

		auto const arg = args[index];
		bool has_apos = arg.find('\'') != std::string::npos;
		bool needs_escape = false;
		for (auto esc : R"(\ ")"sv) {
			needs_escape = arg.find(esc) != std::string::npos;
			if (needs_escape) break;
		}

		if (!has_apos && !needs_escape) {
			fmt::print("{}", arg);
			continue;
		}

		if (!has_apos) {
			fmt::print("'{}'", arg);
			continue;
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
	std::fputc('\n', stdout);
	return 0;
}
