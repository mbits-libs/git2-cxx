// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/args.hh>

namespace cov::app {
	// GCOV_EXCL_START -- linked with CANNOT_INITIALIZE in init
	[[noreturn]] void parser_holder::error(std::error_code const& ec) {
		auto cat_name = ec.category().name();
		if (ec.category() == git::category()) {
			auto const error = git_error_last();
			if (error && error->message && *error->message) {
				fmt::print("{}: Git error: {}\n", parser_.program(),
				           error->message);
				std::exit(2);
			}
			cat_name = "Git";
		}
		fmt::print("{}: {} error {}: {}\n", parser_.program(), cat_name,
		           ec.value(), platform::con_to_u8(ec));
		std::exit(2);
	}
	// GCOV_EXCL_STOP
}  // namespace cov::app