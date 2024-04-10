// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <filesystem>
#include <string>

namespace cov {
	struct init_options {
		enum {
			reinit = 1,
			worktree = 2,
		};
		int flags{};
		std::string branch_name{};
	};

	std::filesystem::path init_repository(std::filesystem::path const& base,
	                                      std::filesystem::path const& git_dir,
	                                      std::error_code&,
	                                      init_options const& = {});
}  // namespace cov
