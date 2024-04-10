// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <cov/app/dirs.hh>
#include <cov/app/split_command.hh>
#include <cov/app/tools.hh>
#include <cov/app/tr.hh>
#include <cov/discover.hh>
#include <cov/error.hh>
#include "../../cov-api/src/cov/path-utils.hh"

namespace cov::app {
	namespace {
		constexpr auto prefix = "alias."sv;

		std::optional<std::string> resolve(git::config const& cfg,
		                                   std::string_view tool) {
			std::optional<std::string> result{};

			cfg.foreach_entry([tool, &result](git_config_entry const* entry) {
				auto view = std::string_view{entry->name};
				if (view.starts_with(prefix) &&
				    view.substr(prefix.length()) == tool) {
					result = entry->value;
				}
				return GIT_OK;
			});

			return result;
		}  // GCOV_EXCL_LINE[GCC]

		int handle_resolved(std::string_view tool,
		                    args::arglist args,
		                    std::filesystem::path const& sysroot,
		                    std::span<builtin_tool const> const& builtins) {
			for (auto const& builtin : builtins) {
				if (builtin.name != tool) continue;
				static constexpr auto tool_prefix = "cov "sv;
				std::string name{};
				name.reserve(tool_prefix.size() + tool.size());
				name.append(tool_prefix);
				name.append(tool);
				return builtin.tool(name, args);
			}  // GCOV_EXCL_LINE[WIN32]

			return platform::run_tool(
			    weakly_canonical(sysroot / directory_info::core), tool, args);
		}

		std::set<std::string> list_tools(tools const& runner,
		                                 std::string_view group,
		                                 std::filesystem::path const& sysroot) {
			if (group == "builtins"sv || group == "builtin"sv)
				return runner.list_builtins();
			if (group == "aliases"sv || group == "alias"sv)
				return runner.list_aliases();
			if (group == "others"sv) return runner.list_externals(sysroot);
			return {};
		}
	}  // namespace

	std::filesystem::path tools::get_sysroot() {
		std::error_code ec{};

		auto sysroot = platform::exec_path();
		if (!sysroot.empty()) {
			sysroot = sysroot.parent_path() / directory_info::root;
			ec.clear();
			if (!is_directory(sysroot / directory_info::core, ec) || ec) {
				[[unlikely]];     // GCOV_EXCL_LINE
				sysroot.clear();  // GCOV_EXCL_LINE
			}
		}

		if (sysroot.empty()) {
			// GCOV_EXCL_START
			[[unlikely]];
			sysroot = directory_info::prefix;
			ec.clear();
			if (!is_directory(sysroot / directory_info::core, ec) || ec)
				sysroot.clear();
			// GCOV_EXCL_STOP
		}

		return sysroot;
	}  // GCOV_EXCL_LINE

	git::config tools::cautiously_open_config(
	    std::filesystem::path const& sysroot,
	    std::filesystem::path const& current_directory) {
		std::error_code ec{};
		auto result =
		    git::config::open_default(sysroot, names::dot_config, "cov"sv, ec);
		if (ec) {
			[[unlikely]];      // GCOV_EXCL_LINE
			result = nullptr;  // GCOV_EXCL_LINE
			return result;     // GCOV_EXCL_LINE
		}                      // GCOV_EXCL_LINE

		// if we are inside a repo, add local config as well...
		if (!current_directory.empty()) {
			auto const directory = discover_repository(current_directory, ec);
			if (!ec) ec = result.add_local_config(directory);
			if (ec == git::errc::notfound ||
			    ec == cov::errc::uninitialized_worktree)
				ec.clear();
		}
		if (ec) result = nullptr;
		return result;
	}

	int tools::handle(std::string_view tool,
	                  std::string& aliased,
	                  args::arglist args,
	                  std::filesystem::path const& sysroot,
	                  CovStrings const& tr) const {
		auto cmd = resolve(cfg_, tool);
		if (!cmd) return handle_resolved(tool, args, sysroot, builtins_);

		for (decltype(args.size()) index = 0; index < args.size(); ++index) {
			if (args[index] == "-h"sv || args[index] == "--help"sv) {
				fmt::print(stderr, fmt::runtime(tr(covlng::HELP_ALIAS)), tool,
				           *cmd);
				return 0;
			}
		}

		std::vector<std::string> combined_args{};

		while (true) {
			auto new_args = split_command(*cmd);

			if (new_args.empty()) {
				fmt::print(stderr, fmt::runtime(tr(covlng::ERROR_ALIAS)), tool);
				return -EINVAL;
			}
			aliased = new_args.front();
			combined_args.insert(
			    combined_args.begin(),
			    std::move_iterator{std::next(new_args.begin())},
			    std::move_iterator{new_args.end()});
			tool = aliased;

			cmd = resolve(cfg_, tool);
			if (!cmd) break;
		}

		combined_args.insert(combined_args.end(), args.data(),
		                     args.data() + args.size());
		std::vector<char*> just_pointers{};
		just_pointers.reserve(combined_args.size());

		std::transform(combined_args.begin(), combined_args.end(),
		               std::back_inserter(just_pointers),
		               [](auto& str) { return str.data(); });

		return handle_resolved(
		    aliased,
		    {static_cast<unsigned>(just_pointers.size()), just_pointers.data()},
		    sysroot, builtins_);
	}

	std::set<std::string> tools::list_aliases() const {
		std::set<std::string, std::less<>> commands{};

		cfg_.foreach_entry([&commands](git_config_entry const* entry) {
			auto name = std::string_view{entry->name};
			if (name.starts_with(prefix)) {
				auto const view = name.substr(prefix.size());
				auto it = commands.lower_bound(view);
				if (it == commands.end() || *it != view)
					commands.insert(it, std::string{view.data(), view.size()});
			}
			return GIT_OK;
		});

		std::set<std::string> result{};
		result.merge(std::move(commands));
		return result;
	}

	std::set<std::string> tools::list_builtins() const {
		std::set<std::string, std::less<>> commands{};

		for (auto const& builtin : builtins_) {
			auto it = commands.lower_bound(builtin.name);
			if (it == commands.end() || *it != builtin.name)
				commands.insert(
				    it, std::string{builtin.name.data(), builtin.name.size()});
		}

		std::set<std::string> result{};
		result.merge(std::move(commands));
		return result;
	}

	std::set<std::string> tools::list_externals(
	    std::filesystem::path const& sysroot) const {
		std::set<std::string, std::less<>> commands{};

		static constexpr auto cov_prefix = "cov-"sv;
#ifdef _WIN32
		static constexpr auto exe = ".exe"sv;
#endif

		/*
		 * sysroot is one of:
		 * - platform::sys_root()
		 * - path{directory_info::prefix}
		 */

		auto const dir = weakly_canonical(sysroot / directory_info::core);
		std::error_code ec{};
		auto dir_iter = directory_iterator{dir, ec};
		if (ec) return {};

		for (auto const& entry : dir_iter) {
			if (is_directory(entry.status())) continue;
			auto const filename = entry.path().filename().string();
			auto view = std::string_view{filename};

#ifdef _WIN32
			if (!view.ends_with(exe)) continue;
			view = view.substr(0, view.length() - exe.length());
#endif
			if (!view.starts_with(cov_prefix)) continue;
			view = view.substr(cov_prefix.length());

			auto it = commands.lower_bound(view);
			if (it == commands.end() || *it != view)
				commands.insert(it, std::string{view.data(), view.size()});
		}

		std::set<std::string> result{};
		result.merge(std::move(commands));
		return result;
	}

	std::set<std::string> tools::list_tools(
	    std::string_view groups,
	    std::filesystem::path const& sysroot) const {
		std::set<std::string> result{};
		auto pos = groups.find(',');
		while (pos != std::string::npos) {
			auto const group = groups.substr(0, pos);
			groups = groups.substr(pos + 1);
			pos = groups.find(',');

			result.merge(app::list_tools(*this, group, sysroot));
		}
		result.merge(app::list_tools(*this, groups, sysroot));
		return result;
	}
}  // namespace cov::app
