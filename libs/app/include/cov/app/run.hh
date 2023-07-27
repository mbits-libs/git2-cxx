// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace cov::app::platform {
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

	std::optional<std::filesystem::path> find_program(
	    std::span<std::string const> names,
	    std::filesystem::path const& hint = {});

	captured_output run(std::filesystem::path const& exec,
	                    args::arglist args,
	                    std::filesystem::path const& cwd,
	                    std::vector<std::byte> const& input);
	captured_output run(std::filesystem::path const& exec,
	                    args::arglist args,
	                    std::vector<std::byte> const& input = {});
	int call(std::filesystem::path const& exec,
	         args::arglist args,
	         std::filesystem::path const& cwd,
	         std::vector<std::byte> const& input);

	int call(std::filesystem::path const& exec,
	         args::arglist args,
	         std::vector<std::byte> const& input = {});
}  // namespace cov::app::platform
