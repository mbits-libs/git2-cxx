// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cov::app {
	std::vector<std::string> split_command(std::string_view command);
}  // namespace cov::app
