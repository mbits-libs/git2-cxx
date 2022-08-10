// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/dirs.hh>
#include <cov/app/rt_path.hh>

namespace cov::app::platform {
	std::filesystem::path const& sys_root() {
		// dirname / ..
		static auto const root = exec_path().parent_path().parent_path();
		return root;
	}

	std::filesystem::path locale_dir() {
		return sys_root() / directory_info::share / "locale"sv;
	}  // GCOV_EXCL_LINE
}  // namespace cov::app::platform
