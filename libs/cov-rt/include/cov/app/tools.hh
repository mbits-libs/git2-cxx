// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <cov/app/cov_tr.hh>
#include <cov/git2/config.hh>
#include <filesystem>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace cov::app {
	struct builtin_tool {
		std::string_view name;
		int (*tool)(std::string_view tool, args::arglist args);
	};

	class tools {
	public:
		tools(git::config&& cfg, std::span<builtin_tool const> const& builtins)
		    : cfg_{std::move(cfg)}, builtins_{builtins} {}

		static std::filesystem::path get_sysroot();
		static git::config cautiously_open_config(
		    std::filesystem::path const& sysroot,
		    std::filesystem::path const& current_directory);

		int handle(std::string_view tool,
		           std::string& aliased,
		           args::arglist args,
		           std::filesystem::path const& sysroot,
		           CovStrings const& tr) const;

		std::set<std::string> list_aliases() const;
		std::set<std::string> list_builtins() const;
		std::set<std::string> list_externals(
		    std::filesystem::path const& sysroot) const;

		std::set<std::string> list_tools(
		    std::string_view groups,
		    std::filesystem::path const& sysroot) const;

	private:
		git::config cfg_;
		std::span<builtin_tool const> builtins_;
	};

	namespace platform {
		int run_tool(std::filesystem::path const& tooldir,
		             std::string_view tool,
		             args::arglist args);

		struct captured_output {
			std::vector<std::byte> output{};
			std::vector<std::byte> error{};
			int return_code{};
		};

		captured_output run_filter(std::filesystem::path const& filter_dir,
		                           std::filesystem::path const& cwd,
		                           std::string_view filter,
		                           args::arglist args,
		                           std::vector<std::byte> const& input);
	}  // namespace platform
}  // namespace cov::app
