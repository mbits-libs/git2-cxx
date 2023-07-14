// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <iostream>
#include <string_view>

namespace cov::testing {
	std::ostream& single_char(std::ostream& out, char c);
	std::ostream& print_view(std::ostream& out, std::string_view text);
	std::ostream& print_char(std::ostream& out, char c);
}  // namespace cov::testing
