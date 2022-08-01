// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace cov::app::report {
	struct file_info {
		std::string name{};
		std::string algorithm{};
		std::string digest{};
		std::map<unsigned, unsigned> line_coverage{};
		auto operator<=>(file_info const&) const noexcept = default;
	};

	struct git_info {
		std::string branch{};
		std::string head{};
		auto operator<=>(git_info const&) const noexcept = default;
	};

	struct report_info {
		git_info git{};
		std::vector<file_info> files{};

		auto operator<=>(report_info const&) const noexcept = default;
		bool load_from_text(std::string_view u8_encoded);
	};
}  // namespace cov::app::report
