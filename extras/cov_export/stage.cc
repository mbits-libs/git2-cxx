// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "stage.hh"
#include <cov/module.hh>
#include <json/json.hpp>
#include <native/path.hh>
#include <set>
#include <web/components.hh>

using namespace std::literals;

namespace cov::app::web {
	void stage::initialize(std::error_code& ec) {
		std::filesystem::remove_all(out_dir, ec);
		if (ec) return;

		std::filesystem::create_directories(out_dir, ec);
		if (ec) return;

		std::filesystem::copy(
		    platform::core_extensions::sys_root() / directory_info::site_html,
		    out_dir, std::filesystem::copy_options::recursive, ec);
		if (ec) return;

		std::tie(commit_ctx, report_ctx) = add_build_info(repo, ref, base, ec);
		if (ec) return;
	}

	std::vector<web::page> stage::list_pages_in_report() const {
		std::set<std::string> modules{}, files{}, dirs{};

		auto sep = mods->separator().empty() ? "/"sv : mods->separator();

		for (auto const& module : mods->entries()) {
			auto mod = std::string_view{module.name};
			auto pos = mod.rfind(sep);
			while (pos != std::string_view::npos) {
				modules.insert({mod.data(), mod.size()});
				mod = mod.substr(0, pos);
				pos = mod.rfind(sep);
			}
			modules.insert({mod.data(), mod.size()});
		}

		for (auto const& stat : diff) {
			files.insert(stat.filename);
			auto const empty_path = std::filesystem::path{};
			auto dir = make_u8path(stat.filename).parent_path();
			while (dir != empty_path) {
				dirs.insert(get_u8path(dir));
				dir = dir.parent_path();
			}
		}

		std::vector<web::page> pages{};
		pages.reserve(1 + modules.size() + files.size() + dirs.size());
		pages.push_back({"index.html", {}, {}});

		for (auto const& module : modules) {
			pages.push_back({links.path_of(projection::entry_type::module,
			                               {.expanded = module}),
			                 module,
			                 {}});
		}
		for (auto const& dir : dirs) {
			pages.push_back({links.path_of(projection::entry_type::directory,
			                               {.expanded = dir}),
			                 {},
			                 dir});
		}
		for (auto const& file : files) {
			pages.push_back({links.path_of(projection::entry_type::file,
			                               {.expanded = file}),
			                 {},
			                 file});
		}

		return pages;
	}

	mstch::map stage::page_state::create_context(stage const& stg,
	                                             std::error_code& ec) {
		auto view = projection::report_filter{stg.mods.get(), pg.module_filter,
		                                      pg.fname_filter};
		auto entries = view.project(stg.diff, &stg.repo);

		mstch::map ctx{};
		auto const is_standalone =
		    add_page_context(ctx, view, entries, stg.ref, stg.marks, stg.repo,
		                     stg.commit_ctx, stg.report_ctx, stg.octicons,
		                     stg.replacements, true, stg.links, ec);
		if (ec) return {};

		template_name = is_standalone ? "file.html"s : "listing.html"s;

		return ctx;
	}

	stage::page_state stage::next_page(page const& pg, std::error_code& ec) {
		auto const [filename, module_filter, fname_filter] = pg;
		links.adjust_root(pg.filename);
		page_state state{.pg = pg, .full_path = out_dir / filename};

		std::filesystem::create_directories(state.full_path.parent_path(), ec);
		return state;
	}

	std::filesystem::path stage::runtime_site() {
		return platform::core_extensions::sys_root() / directory_info::site_res;
	}

	std::filesystem::path stage::installed_site() {
		return std::filesystem::path{directory_info::prefix} /
		       directory_info::site_res;
	}

#ifndef NDEBUG
	std::filesystem::path stage::build_site() {
		return make_u8path(directory_info::build) / directory_info::site_res;
	}

	std::filesystem::path stage::source_site() {
		return make_u8path(directory_info::source) / "data/html/res";
	}
#endif
}  // namespace cov::app::web
