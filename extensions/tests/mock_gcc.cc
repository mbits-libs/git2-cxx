// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>
#include <string_view>

using namespace std::literals;

int main(int argc, char** argv) {
	auto const args = ::args::from_main(argc, argv);
	static constexpr auto version = "17.0.1"sv;
	fmt::print("{0} (Chell {1}-D) {1}\n", args.progname, version);
}
