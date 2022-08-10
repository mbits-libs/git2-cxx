// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>

namespace cov::app::platform {
	std::filesystem::path const& sys_root();
	std::filesystem::path locale_dir();
}  // namespace cov::app::platform
