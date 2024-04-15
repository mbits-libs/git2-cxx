// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "platform.hh"
#include <fmt/format.h>
#include <cov/app/dirs.hh>

using namespace std::literals;

namespace cov::app::platform {
	std::filesystem::path const& sys_root() {
		static auto const root = [] {
			auto const dir = exec_path().parent_path();
			// share/cov-XY/filters
			return dir.parent_path().parent_path().parent_path();
		}();

		return root;
	}

	std::filesystem::path locale_dir() {
		return sys_root() / directory_info::share / "locale"sv;
	}  // GCOV_EXCL_LINE

	std::vector<char8_t> read_input() {
		std::vector<char8_t> result{};
		char8_t buffer[8192];

		while (auto read = std::fread(buffer, 1, sizeof(buffer), stdin)) {
			result.insert(result.end(), buffer, buffer + read);
		}

		return result;
	}  // GCOV_EXCL_LINE
};  // namespace cov::app::platform
