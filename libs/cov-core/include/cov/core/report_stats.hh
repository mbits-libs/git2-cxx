// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
#pragma once

#include <cov/format.hh>
#include <cov/projection.hh>
#include <string>
#include <utility>
#include <vector>

namespace cov::core {
	enum class col_title {
		branches_covered,
		branches_missing,
		functions_covered,
		functions_missing,
		lines_covered,
		lines_missing,
		lines_visited,
		lines_relevant,
		lines_total,
	};

	enum class col_priority {
		supplemental,
		key,
		high,
	};

	enum class col_data {
		percentage,
		counter,
	};

	struct column_info {
		col_title title{};
		col_priority priority{};
		col_data data_type{};

		template <col_title Title, col_priority Priority, col_data DataType>
		static consteval column_info from() noexcept {
			return {
			    .title = Title,
			    .priority = Priority,
			    .data_type = DataType,
			};
		}
	};

	enum class value_category {
		passing,
		incomplete,
		failing,
	};

	struct cell_info {
		std::string value{};
		std::string change{};
		value_category category{};
		bool change_is_negative{false};
	};

	struct row_info {
		projection::entry_type type{};
		projection::label name{};
		std::vector<cell_info> data{};
	};

	struct projected_entries {
		std::vector<column_info> columns{};
		std::vector<row_info> rows{};
		std::vector<cell_info> footer{};
	};

	enum class with : unsigned {
		branches = 0x0001,
		functions = 0x0002,
		branches_missing = 0x0004,
		functions_missing = 0x0008,
		lines_missing = 0x0010,
	};

	inline with operator|(with lhs, with rhs) noexcept {
		return static_cast<with>(std::to_underlying(lhs) |
		                         std::to_underlying(rhs));
	}

	inline with& operator|=(with& lhs, with rhs) noexcept {
		lhs = lhs | rhs;
		return lhs;
	}

	inline with operator~(with lhs) noexcept {
		return static_cast<with>(~std::to_underlying(lhs));
	}

	inline with operator&(with lhs, with rhs) noexcept {
		return static_cast<with>(std::to_underlying(lhs) &
		                         std::to_underlying(rhs));
	}

	struct stats {
		static std::pair<projection::entry_stats, with> calc_stats(
		    std::vector<projection::entry> const& entries);
		static projected_entries project(
		    placeholder::rating const& marks,
		    std::vector<projection::entry> const& entries,
		    projection::entry_stats const& total,
		    with flags);
	};
}  // namespace cov::core
