// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <filesystem>

namespace cov {
	enum class discover { within_fs = false, across_fs = true };

	std::filesystem::path discover_repository(std::filesystem::path const&,
	                                          discover across_fs,
	                                          std::error_code& ec);

	inline std::filesystem::path discover_repository(
	    std::filesystem::path const& current_dir,
	    std::error_code& ec) {
		return discover_repository(current_dir, discover::within_fs, ec);
	}
}  // namespace cov
