// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <cov/app/tools.hh>

using namespace std::literals;

namespace cov::app::root {
	struct help_command {
		std::string_view name;
		std::string_view description;
	};

	struct help_group {
		std::string_view name;
		std::span<help_command const> commands;
	};

	class parser {
	public:
		parser(args::args_view const& arguments,
		       std::span<builtin_tool const> const& builtins);
		args::args_view parse();
		void noent(std::string_view tool) const;

	private:
		args::parser setup_parser(args::args_view const& arguments);
		args::null_translator null_tr_{};
		args::parser parser_;
		std::span<builtin_tool const> builtins_;
	};

	tools setup_tools(std::span<builtin_tool const> const& builtins);
}  // namespace cov::app::root
