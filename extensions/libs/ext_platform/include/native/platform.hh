// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <vector>

namespace cov::app::platform {
	std::filesystem::path const& sys_root();
	std::filesystem::path locale_dir();
	std::vector<char8_t> read_input();
};  // namespace cov::app::platform
