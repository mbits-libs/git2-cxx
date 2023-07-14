// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#define NOMINMAX

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/cvg_info.hh>
#include <cov/app/line_printer.hh>
#include <cov/app/path.hh>
#include <cov/app/show.hh>
#include <cov/format.hh>
#include <cov/git2/blob.hh>
#include <cov/git2/repository.hh>
#include <cov/module.hh>
#include <hilite/hilite.hh>
#include <hilite/lighter.hh>
#include <hilite/none.hh>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace cov::app::builtin::show {
	std::vector<file_stats> report_diff(app::show::parser::response const& info,
	                                    std::error_code& ec) {
		auto const newer = info.repo.lookup<cov::report>(info.range.to, ec);
		if (ec) return {};

		if (info.range.single) {
			return info.repo.diff_with_parent(newer, ec);
		}

		auto const older = info.repo.lookup<cov::report>(info.range.from, ec);
		if (ec) return {};

		return info.repo.diff_betwen_reports(newer, older, ec);
	}

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		using placeholder::color;

		app::show::parser p{{tool, args},
		                    {platform::locale_dir(), ::lngs::system_locales()}};
		auto info = p.parse();
		std::error_code ec{};

		auto mods = cov::modules::from_report(info.range.to, info.repo, ec);
		if (ec) {
			if (ec == git::errc::notfound)
				ec = {};
			else                      // GCOV_EXCL_LINE
				p.error(ec, p.tr());  // GCOV_EXCL_LINE
		}

		auto view = projection::report_filter{
		    mods.get(), p.module.value_or(std::string{}), info.path};
		auto entries = view.project(report_diff(info, ec));
		if (ec) p.error(ec, p.tr());

		app::show::context ctx{
		    .colorizer = p.colorizer(),
		    .marks = placeholder::context::rating_from(info.repo)};

		auto const is_standalone =
		    entries.size() == 1 &&
		    entries.front().type == projection::entry_type::standalone_file;

		auto const is_root = !is_standalone && view.module.filter.empty() &&
		                     view.fname.prefix.empty();

		if (is_root) {
			if (!info.range.single &&
			    git_oid_equal(&info.range.from, &info.range.to)) {
				info.range.single = true;
				info.range.from = {};
			}

			p.show.print(info.repo, info.range, 1);
		} else if (is_standalone) {
			fmt::print("{}file {}{}\n\n", ctx.color_for(color::yellow),
			           entries.front().name.expanded,
			           ctx.color_for(color::reset));
		} else {
			if (!view.module.filter.empty()) {
				fmt::print("{}module {}{}\n", ctx.color_for(color::yellow),
				           view.module.filter, ctx.color_for(color::reset));
			}
			if (!view.fname.prefix.empty()) {
				fmt::print("{}directory {}{}\n", ctx.color_for(color::yellow),
				           view.fname.prefix, ctx.color_for(color::reset));
			}
			if (!view.module.filter.empty() || !view.fname.prefix.empty())
				fmt::print("\n");
		}

		ctx.print_table(entries);

		if (!is_standalone) return 0;

		auto const report = info.repo.lookup<cov::report>(info.range.to, ec);

		if (report->file_list_id().is_zero()) return 1;
		auto const files =
		    info.repo.lookup<cov::files>(report->file_list_id(), ec);
		if (ec) p.error(ec, p.tr());

		auto const* file_entry = files->by_path(entries.front().name.expanded);
		if (!file_entry || file_entry->contents().is_zero()) return 1;

		cvg_info cvg{};

		if (!file_entry->line_coverage().is_zero()) {
			auto const file_cvg = info.repo.lookup<cov::line_coverage>(
			    file_entry->line_coverage(), ec);
			if (file_cvg && !ec)
				cvg = cvg_info::from_coverage(file_cvg->coverage());
		}

		auto const data = file_entry->get_contents(info.repo, ec);
		if (ec) p.error(ec, p.tr());
		cvg.load_syntax(
		    std::string_view{reinterpret_cast<char const*>(data.data()),
		                     data.size()},
		    entries.front().name.display);

		auto clr = p.show.color_type;
		if (clr == use_feature::automatic) {
			clr = cov::platform::is_terminal(stdout) ? use_feature::yes
			                                         : use_feature::no;
		}

		auto const widths = cvg.column_widths();

		bool first = true;
		for (auto const& [start, stop] : cvg.chunks) {
			if (first)
				first = false;
			else
				fmt::print("\n");
			fmt::print("\n");

			bool ran_out_of_lines{false};
			for (auto line_no = start; line_no <= stop; ++line_no) {
				if (!cvg.has_line(line_no)) {
					ran_out_of_lines = true;
					break;
				}
				fmt::print("{}\n", cvg.to_string(line_no, widths,
				                                 clr == use_feature::yes));
			}
			if (ran_out_of_lines) break;
		}

		fmt::print("\n");
		return 0;
	}
}  // namespace cov::app::builtin::show
