// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/core/report_stats.hh>
#include "column_selectors.hh"

namespace cov::core {
	using namespace projection;

	namespace {
		using vis_callback = bool (*)(with);
		bool has(with flags, with test) { return (flags & test) == test; }
		bool always(with) { return true; }
		bool with_branches(with flags) { return has(flags, with::branches); }
		bool with_funcs(with flags) { return has(flags, with::functions); }
		bool with_branches_missing(with flags) {
			return has(flags, with::branches | with::branches_missing);
		}
		bool with_funcs_missing(with flags) {
			return has(flags, with::functions | with::functions_missing);
		}
		bool with_lines_missing(with flags) {
			return has(flags, with::lines_missing);
		}

		struct column {
			column_info info;
			selector_factory selector;
			vis_callback vis{always};
		};

		static constexpr std::array columns = {
		    // GCOV_EXCL_START for now
		    column{
		        column_info::from<col_title::branches_covered,
		                          col_priority::high,
		                          col_data::percentage>(),
		        get_ratio_sel<placeholder::stats::branches_percent>(
		            [](auto const& stats) { return stats.branches; }),
		        with_branches,
		    },
		    column{
		        column_info::from<col_title::branches_relevant,
		                          col_priority::supplemental,
		                          col_data::counter>(),
		        get_counter_sel<change_is::in_line_with_sign>(
		            [](auto const& stats) { return stats.branches.relevant; }),
		        with_branches,
		    },
		    column{
		        column_info::from<col_title::branches_missing,
		                          col_priority::key,
		                          col_data::counter>(),
		        get_dimmed_counter_sel<change_is::reversed>(
		            [](auto const& stats) {
			            return stats.branches.relevant - stats.branches.visited;
		            }),
		        with_branches_missing,
		    },
		    // GCOV_EXCL_STOP
		    column{
		        column_info::from<col_title::functions_covered,
		                          col_priority::high,
		                          col_data::percentage>(),
		        get_ratio_sel<placeholder::stats::functions_percent>(
		            [](auto const& stats) { return stats.functions; }),
		        with_funcs,
		    },
		    column{
		        column_info::from<col_title::functions_relevant,
		                          col_priority::supplemental,
		                          col_data::counter>(),
		        get_counter_sel<change_is::in_line_with_sign>(
		            [](auto const& stats) { return stats.functions.relevant; }),
		        with_funcs,
		    },
		    column{
		        column_info::from<col_title::functions_missing,
		                          col_priority::key,
		                          col_data::counter>(),
		        get_dimmed_counter_sel<change_is::reversed>(
		            [](auto const& stats) {
			            return stats.functions.relevant -
			                   stats.functions.visited;
		            }),
		        with_funcs_missing,
		    },
		    column{
		        column_info::from<col_title::lines_covered,
		                          col_priority::high,
		                          col_data::percentage>(),
		        get_ratio_sel<placeholder::stats::lines_percent>(
		            [](auto const& stats) { return stats.lines; }),
		    },
		    column{
		        column_info::from<col_title::lines_visited,
		                          col_priority::key,
		                          col_data::counter>(),
		        get_counter_sel<change_is::in_line_with_sign>(
		            [](auto const& stats) { return stats.lines.visited; }),
		    },
		    column{
		        column_info::from<col_title::lines_relevant,
		                          col_priority::supplemental,
		                          col_data::counter>(),
		        get_counter_sel<change_is::in_line_with_sign>(
		            [](auto const& stats) { return stats.lines.relevant; }),
		    },
		    column{
		        column_info::from<col_title::lines_missing,
		                          col_priority::key,
		                          col_data::counter>(),
		        get_dimmed_counter_sel<change_is::reversed>(
		            [](auto const& stats) {
			            return stats.lines.relevant - stats.lines.visited;
		            }),
		        with_lines_missing,
		    },
		    column{
		        column_info::from<col_title::lines_total,
		                          col_priority::supplemental,
		                          col_data::counter>(),
		        get_counter_sel<change_is::in_line_with_sign>(
		            [](auto const& stats) { return stats.lines_total; }),
		    },
		};

		std::vector<cell_info> cells_from_stats(
		    placeholder::rating const& marks,
		    entry_stats const& stats,
		    with flags,
		    size_t column_size) {
			std::vector<cell_info> result{};
			result.reserve(column_size);
			auto const change = stats.diff();

			for (auto const& col : columns) {
				if (!col.vis(flags)) continue;

				auto const selector = col.selector;
				if (selector == nullptr) {
					result.push_back({});
					continue;
				}

				result.push_back(selector().cell(marks, change));
			}

			return result;
		}  // GCOV_EXCL_LINE
	}      // namespace

	std::pair<entry_stats, with> stats::calc_stats(
	    std::vector<projection::entry> const& entries) {
		std::pair<entry_stats, with> result{{}, {}};
		auto& [total, flags] = result;

		for (auto const& entry : entries) {
			total.extend(entry.stats);

			if (entry.stats.current.lines.relevant !=
			    entry.stats.current.lines.visited)
				flags |= with::lines_missing;

			if (entry.stats.current.functions.relevant !=
			    entry.stats.current.functions.visited)
				flags |= with::functions_missing;

			if (entry.stats.current.branches.relevant !=
			    entry.stats.current.branches.visited)
				flags |= with::branches_missing;  // GCOV_EXCL_LINE for now
		}

		if (total.current.branches.relevant > 0) flags |= with::branches;
		if (total.current.functions.relevant > 0) flags |= with::functions;

		return result;
	}

	projected_entries stats::project(placeholder::rating const& marks,
	                                 std::vector<entry> const& entries,
	                                 entry_stats const& total,
	                                 with flags) {
		projected_entries result{};

		result.columns.reserve(columns.size());
		result.rows.reserve(entries.size());

		for (auto const& col : columns) {
			if (!col.vis(flags)) continue;
			result.columns.push_back(col.info);
		}

		for (auto const& entry : entries) {
			result.rows.push_back({
			    .type = entry.type,
			    .name = entry.name,
			    .data = cells_from_stats(marks, entry.stats, flags,
			                             result.columns.size()),
			});
		}

		if (result.rows.size() > 1) {
			result.footer =
			    cells_from_stats(marks, total, flags, result.columns.size());
		}

		return result;
	}
}  // namespace cov::core
