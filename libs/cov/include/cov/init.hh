// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <filesystem>

namespace cov {
	struct init_options {
		enum {
			reinit = 1,
		};
		int flags{};
	};

	std::filesystem::path init_repository(std::filesystem::path const& base,
	                                      std::filesystem::path const& git_dir,
	                                      std::error_code&,
	                                      init_options const& = {});
}  // namespace cov
