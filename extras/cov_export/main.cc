// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <c++filt/json.hh>
#include <c++filt/parser.hh>
#include <cov/core/report_stats.hh>
#include <cov/format.hh>
#include <cov/io/file.hh>
#include <cov/module.hh>
#include <native/path.hh>
#include <set>
#include <web/link_service.hh>
#include "parser.hh"
#include "stage.hh"

using namespace std::literals;

namespace cov::app::report_export {
	std::vector<file_stats> report_diff(parser::response const& info,
	                                    std::error_code& ec) {
		if (info.range.single) {
			std::error_code ec1{}, ec2{};
			auto const build = info.repo.lookup<cov::build>(info.range.to, ec1);
			auto files = info.repo.lookup<cov::files>(info.range.to, ec2);
			if (!ec1 && build) {
				ec2.clear();
				files =
				    info.repo.lookup<cov::files>(build->file_list_id(), ec2);
			}
			if (!ec2 && files) {
				std::vector<file_stats> result{};
				result.reserve(files->entries().size());
				for (auto const& entry : files->entries()) {
					auto const path_view = entry->path();
					auto path = std::string{path_view.data(), path_view.size()};
					result.push_back({
					    .filename = std::move(path),
					    .current = entry->stats(),
					    .previous = entry->stats(),
					    .diff_kind = file_diff::normal,
					    .current_functions = entry->function_coverage(),
					    .previous_functions = entry->function_coverage(),
					});
				}
				return result;
			}
		}

		auto const newer = info.repo.lookup<cov::report>(info.range.to, ec);
		if (ec) return {};

		if (info.range.single) {
			return info.repo.diff_with_parent(newer, ec);
		}

		auto const older = info.repo.lookup<cov::report>(info.range.from, ec);
		if (ec) return {};

		return info.repo.diff_betwen_reports(newer, older, ec);
	}

	struct counted_actions {
		size_t count;
		size_t index{1};
		size_t prev_line_length{0};

		[[noreturn]] void error(parser& p, std::error_code const& ec) {
			if (cov::platform::is_terminal(stdout)) fmt::print("\n");
			p.error(ec, p.tr());
		}

		void on_action(std::string_view action) {
			if (cov::platform::is_terminal(stdout)) {
				fmt::print("\r\033[2K[{}/{}] {}", index, count, action);
				std::fflush(stdout);
			} else {
				fmt::print("[{}/{}] {}\n", index, count, action);
			}
			++index;
		}
		~counted_actions() {
			if (cov::platform::is_terminal(stdout)) fmt::print("\n");
		}
	};

	void html_report(web::stage stage, parser& p) {
		std::error_code ec{};

		stage.initialize(ec);
		if (ec) p.error(ec, p.tr());

		auto const pages = stage.list_pages_in_report();
		counted_actions logger{.count = pages.size()};

		for (auto const& item : pages) {
			logger.on_action(get_u8path(item.filename));
			auto state = stage.next_page(item, ec);
			if (ec) logger.error(p, ec);

			auto ctx = state.create_context(stage, ec);
			if (ec) logger.error(p, ec);

			auto page_text = stage.tmplt.render(state.template_name, ctx);

			auto out = io::fopen(state.full_path, "wb");
			if (!out) {
				auto const error = errno;
				ec = std::make_error_code(error ? static_cast<std::errc>(error)
				                                : std::errc::permission_denied);
				logger.error(p, ec);
			}

			out.store(page_text.c_str(), page_text.size());
		}
	}

	char const* get_home() noexcept {
		if (auto var = std::getenv("HOME"); var && *var) {
			return var;
		}
		if (auto var = std::getenv("USERPROFILE"); var && *var) {
			return var;
		}
		return nullptr;
	}

	std::vector<std::filesystem::path> replacement_paths(
	    cov::repository const& repo) {
		std::vector<std::filesystem::path> result{};

		auto const system = platform::core_extensions::sys_root();
		auto const home_var = get_home();
		auto const home = home_var ? std::filesystem::path{home_var}
		                           : std::filesystem::path{};
		auto const maybe_local = repo.git_work_dir();
		auto const local =
		    maybe_local ? make_u8path(*maybe_local) : std::filesystem::path{};

		{
			std::error_code ec{};
			auto it = std::filesystem::directory_iterator{
			    system / directory_info::share / "c++filt"sv, ec};
			if (!ec) {
				for (auto const& entry : it) {
					if (entry.is_regular_file() &&
					    entry.path().extension() == ".json"sv) {
						result.push_back(entry.path());
					}
				}
			}
		}

		repo.config().get_multivar_foreach(
		    "filter.path", nullptr, [&](git_config_entry const* entry) -> int {
			    std::filesystem::path const* root = nullptr;
			    switch (entry->level) {
				    case GIT_CONFIG_LEVEL_PROGRAMDATA:
				    case GIT_CONFIG_LEVEL_SYSTEM:
					    root = &system;
					    break;
				    case GIT_CONFIG_LEVEL_XDG:
				    case GIT_CONFIG_LEVEL_GLOBAL:
					    root = &home;
					    break;
				    case GIT_CONFIG_LEVEL_LOCAL:
					    root = &local;
					    break;
				    default:
					    break;
			    }
			    if (!root || root->empty()) return 0;
			    result.push_back(*root / make_u8path(entry->value));
			    return 0;
		    });

		if (!local.empty()) {
			std::error_code ec{};
			auto it =
			    std::filesystem::directory_iterator{local / ".c++filt"sv, ec};
			if (!ec) {
				for (auto const& entry : it) {
					if (entry.is_regular_file() &&
					    entry.path().extension() == ".json"sv) {
						result.push_back(entry.path());
					}
				}
			}
		}

		return result;
	}

	cxx_filt::Replacements load_replacements(cov::repository const& repo) {
		cxx_filt::Replacements result{};

		for (auto const& path : replacement_paths(repo)) {
			auto file = io::fopen(path);
			if (!file) continue;
			fmt::print("c++filt: {}\n", get_u8path(path));
			auto const contents = file.read();
			cxx_filt::append_replacements(
			    {reinterpret_cast<const char*>(contents.data()),
			     contents.size()},
			    result);
		}

		return result;
	}

	int handle(args::args_view const& args) {
		parser p{args,
		         {platform::core_extensions::locale_dir(),
		          ::lngs::system_locales()}};
		auto info = p.parse();

		std::error_code ec{};
		auto mods = cov::modules::from_report(info.range.to, info.repo, ec);
		if (ec) {
			if (ec == git::errc::notfound) {
				ec = {};
			} else if (ec == cov::errc::wrong_object_type) {
				ec.clear();
				mods = cov::modules::make_modules("/"s, {});
			} else {                  // GCOV_EXCL_LINE
				p.error(ec, p.tr());  // GCOV_EXCL_LINE
			}                         // GCOV_EXCL_LINE
		}

		if (info.only_json) {
			p.error("--json is not implemented yet");
		}

		auto diff = report_diff(info, ec);
		if (ec) p.error(ec, p.tr());

		struct mod {
			std::vector<std::string> prefixes, files;
		};

		std::map<std::string, mod> module_mapping{};

		for (auto const& mod : mods->entries()) {
			module_mapping[mod.name].prefixes = mod.prefixes;
		}

		for (auto const& item : diff) {
			size_t length{};
			std::string module_name{};
			for (auto const& entry : mods->entries()) {
				for (std::string_view prefix : entry.prefixes) {
					if (!prefix.empty() && prefix.back() == '/')
						prefix = prefix.substr(0, prefix.length() - 1);
					if (item.filename.starts_with(prefix) &&
					    (item.filename.length() == prefix.length() ||
					     item.filename[prefix.length()] == '/') &&
					    length < prefix.length()) {
						length = prefix.length();
						module_name = entry.name;
					}
				}
			}

			module_mapping[module_name].files.push_back(item.filename);
		}

		html_report({.marks = placeholder::environment::rating_from(info.repo),
		             .mods = std::move(mods),
		             .diff = std::move(diff),
		             .out_dir = info.path,
		             .repo = info.repo,
		             .ref = info.range.to,
		             .base = info.range.from,
		             .replacements = load_replacements(info.repo)},
		            p);

		return 0;
	}
}  // namespace cov::app::report_export

int tool(args::args_view const& args) {
	return cov::app::report_export::handle(args);
}
