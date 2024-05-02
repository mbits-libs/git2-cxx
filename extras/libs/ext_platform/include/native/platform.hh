// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <vector>

namespace cov::app::platform {
	struct locale_dir_provider {
		static std::filesystem::path locale_dir_inside(
		    std::filesystem::path const& sys_root);
	};

	template <typename Final>
	struct sys_provider : locale_dir_provider {
		static std::filesystem::path locale_dir() {
			return locale_dir_provider::locale_dir_inside(Final::sys_root());
		}  // GCOV_EXCL_LINE
	};

	struct filters : sys_provider<filters> {
		static std::filesystem::path const& sys_root();
	};

	struct core_extensions : sys_provider<core_extensions> {
		static std::filesystem::path const& sys_root();
	};

	std::vector<char8_t> read_input();
};  // namespace cov::app::platform
