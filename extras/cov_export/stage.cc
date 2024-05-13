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
	namespace {
		inline std::string from_u8s(std::u8string_view str) {
			return {reinterpret_cast<char const*>(str.data()), str.size()};
		}

		inline std::u8string to_u8s(std::string_view str) {
			return {reinterpret_cast<char8_t const*>(str.data()), str.size()};
		}

		struct json_visitor {
			json::node operator()(auto const&) const { return nullptr; }
			json::node operator()(std::string const& v) const {
				return to_u8s(v);
			}
			json::node operator()(long long v) const { return v; }
			json::node operator()(double v) const { return v; }
			json::node operator()(bool v) const { return v; }
			json::node operator()(mstch::array const& v) const {
				json::array a{};
				a.reserve(v.size());
				for (auto const& item : v) {
					a.push_back(std::visit(json_visitor{}, item.base()));
				}
				return a;
			}
			json::node operator()(mstch::map const& v) const {
				json::map m{};
				for (auto const& [key, item] : v) {
					m.set(to_u8s(key), std::visit(json_visitor{}, item.base()));
				}
				return m;
			}
			json::node operator()(
			    std::shared_ptr<mstch::callback> const& v) const {
				json::map m{};
				auto const keys = v->debug_all_keys();
				for (auto const& key : keys) {
					m.set(to_u8s(key), std::visit(json_visitor{}, v->at(key)));
				}
				return m;
			}
		};
	}  // namespace

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
		std::error_code inner_ec{};
		auto view = projection::report_filter{stg.mods.get(), pg.module_filter,
		                                      pg.fname_filter};
		auto entries = view.project(stg.diff, &stg.repo);
		if (ec) return {};

		auto const is_standalone =
		    entries.size() == 1 &&
		    entries.front().type == projection::entry_type::standalone_file;

		template_name = is_standalone ? "file.html"s : "listing.html"s;

		auto const is_root = !is_standalone && view.module.filter.empty() &&
		                     view.fname.prefix.empty();

		auto const [total, flags] = core::stats::calc_stats(entries);
		auto const table =
		    core::stats::project(stg.marks, entries, total, flags);

		std::string title;
		std::string path;
		bool is_module = false;
		if (is_root) {
			title = "repository"s;
		} else if (is_standalone) {
			auto const& name = entries.front().name;
			title = name.expanded;
			path = title;
		} else if (!view.module.filter.empty()) {
			title = fmt::format("[{}]", view.module.filter);
			path = view.module.filter;
			is_module = true;
		} else if (!view.fname.prefix.empty()) {
			title = view.fname.prefix;
			title.pop_back();
			path = title;
		}

		auto mark = formatter::apply_mark(total.current.lines, stg.marks.lines);

		mstch::map ctx;
		{
			mstch::map build;
			add_mark(build, mark);
			build["commit"] = stg.commit_ctx;
			build["report"] = stg.report_ctx;
			ctx["build"] = std::move(build);
		}

		add_navigation(ctx, is_root, is_module, is_standalone, path, stg.links);
		add_table(ctx, table, stg.links);
		ctx["title"] = std::string{title.data(), title.size()};
		ctx["site-root"] = stg.links.resource_link("");
		ctx["octicons"] = stg.octicons;

		if (is_standalone) {
			add_file_source(ctx, stg.repo, stg.ref,
			                entries.front().name.expanded, stg.replacements,
			                ec);
			if (ec) return {};
		}

		json::string json_context{};
		{
			mstch::node node{ctx};
			auto jsn = std::visit(json_visitor{}, node.base());
			json::write_json(json_context, jsn, json::four_spaces);
		}
		ctx["json"] = from_u8s(json_context);

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
