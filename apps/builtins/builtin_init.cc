// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/parser.hpp>
#include <cov/app/cov_init_tr.hh>

namespace cov::app::builtin::init {
	int handle(std::string_view tool, args::arglist args) {
		fmt::print("Hello, \"{}", tool);
		for (auto str : std::span{args.data(), args.size()}) {
			fmt::print(" {}", str);
		}
		std::fputs("\"!\n", stdout);
		return 0;
	}
}  // namespace cov::app::builtin::init
