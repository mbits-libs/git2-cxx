// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <string_view>

using namespace std::literals;

int main(int, char**) {
	static constexpr auto version = "17.0.1"sv;
	fmt::print(
	    "Chell clang version {}\n"
	    "Target: hal9000-lcars-GLaDOS\n"
	    "Thread model: Elim Garak\n",
	    version);
}
