// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#define NOMINMAX

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/path.hh>
#include <cov/app/show.hh>
#include <cov/module.hh>
#include <git2/repository.hh>
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
		auto const info = p.parse();
		std::error_code ec{};

		auto mods = cov::modules::from_report(info.range.to, info.repo, ec);
		if (ec) {
			if (ec == git::errc::notfound)
				ec = {};
			else
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

		ctx.print(entries);

		return 0;
	}
}  // namespace cov::app::builtin::show
