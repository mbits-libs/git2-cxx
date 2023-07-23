// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/io/types.hh>
#include <cov/report.hh>
#include <hilite/lighter.hh>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cov::app {
	struct view_columns {
		size_t line_no_width;
		size_t count_width;
	};

	struct aliased_name {
		std::string_view label;
		unsigned count{};
	};

	struct cvg_info {
		std::map<unsigned, unsigned> coverage{};
		std::vector<cov::function_coverage::function> functions{};
		std::vector<std::pair<unsigned, unsigned>> chunks{};
		std::string_view file_text{};
		lighter::highlights syntax{};

		function_coverage::function_iterator funcs() const noexcept {
			return {functions};
		}

		static cvg_info from_coverage(
		    std::span<io::v1::coverage const> const& lines);
		void add_functions(cov::function_coverage const& functions);
		void find_chunks();
		void load_syntax(std::string_view text, std::string_view filename);
		view_columns column_widths() const noexcept;

		std::optional<unsigned> max_count() const noexcept;
		std::optional<unsigned> count_for(unsigned line_no) const noexcept;

		bool has_line(size_t line_no) const noexcept {
			return syntax.lines.size() > line_no;
		}
		static std::vector<aliased_name> soft_alias(
		    cov::function_coverage::function const& fn);
		std::string to_string(aliased_name const& fn,
		                      view_columns const& widths,
		                      bool use_color,
		                      size_t max_width,
		                      unsigned total_count) const;
		std::string to_string(size_t line_no,
		                      view_columns const& widths,
		                      bool use_color) const;
	};
}  // namespace cov::app
