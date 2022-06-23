// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <filesystem>

namespace cov {
	enum class discover { within_fs = false, across_fs = true };

	std::filesystem::path discover_repository(
	    std::filesystem::path const&,
	    discover across_fs = discover::within_fs);
}  // namespace cov
