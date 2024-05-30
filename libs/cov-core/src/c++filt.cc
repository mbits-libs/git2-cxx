// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <c++filt/json.hh>
#include <c++filt/parser.hh>
#include <cov/app/dirs.hh>
#include <cov/app/path.hh>
#include <cov/core/c++filt.hh>
#include <cov/io/file.hh>

using namespace std::literals;

namespace cov::core {
	namespace {

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
		    std::filesystem::path const& system,
		    cov::repository const& repo) {
			std::vector<std::filesystem::path> result{};

			auto const home_var = get_home();
			auto const home = home_var ? std::filesystem::path{home_var}
			                           : std::filesystem::path{};
			auto const maybe_local = repo.git_work_dir();
			auto const local = maybe_local ? app::make_u8path(*maybe_local)
			                               : std::filesystem::path{};

			{
				std::error_code ec{};
				auto it = std::filesystem::directory_iterator{
				    system / app::directory_info::share / "c++filt"sv, ec};
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
			    "filter.path", nullptr,
			    [&](git_config_entry const* entry) -> int {
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
					    default:    // GCOV_EXCL_LINE
						    break;  // GCOV_EXCL_LINE
				    }
				    if (!root || root->empty()) return 0;
				    result.push_back(*root / app::make_u8path(entry->value));
				    return 0;
			    });

			if (!local.empty()) {
				std::error_code ec{};
				auto it = std::filesystem::directory_iterator{
				    local / ".c++filt"sv, ec};
				if (!ec) {
					for (auto const& entry : it) {
						if (entry.is_regular_file() &&
						    entry.path().extension() == ".json"sv) {
							result.push_back(entry.path());
						}
					}  // GCOV_EXCL_LINE
				}
			}

			std::sort(result.begin(), result.end());
			return result;
		}  // GCOV_EXCL_LINE
	}      // namespace

	cxx_filt::Replacements load_replacements(
	    std::filesystem::path const& system,
	    cov::repository const& repo,
	    bool debug) {
		cxx_filt::Replacements result{};

		for (auto const& path : replacement_paths(system, repo)) {
			auto file = io::fopen(path);
			if (!file) continue;
			if (debug) fmt::print("c++filt: {}\n", app::get_u8path(path));
			auto const contents = file.read();
			cxx_filt::append_replacements(
			    {reinterpret_cast<const char*>(contents.data()),
			     contents.size()},
			    result);
		}

		return result;
	}  // GCOV_EXCL_LINE

};  // namespace cov::core
