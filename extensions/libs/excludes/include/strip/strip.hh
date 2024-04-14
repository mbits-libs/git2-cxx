// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <json/json.hpp>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "excludes.hh"

namespace cov::app::strip {
	exclude_result find_blocks(std::string const& path,
	                           std::span<std::string_view const> valid_markers,
	                           std::string_view text);

	bool is_line_excluded(unsigned line_no,
	                      std::span<excl_block const> const& excludes);

	std::vector<exclude_report_line> find_lines(
	    json::map& line_coverage,
	    std::span<excl_block const> const& excludes,
	    std::string_view text);

	enum which_lines { soft = false, hard = true };

	unsigned erase_lines(json::map& line_coverage,
	                     std::span<excl_block const> excludes,
	                     std::set<unsigned> const& empties);
}  // namespace cov::app::strip
