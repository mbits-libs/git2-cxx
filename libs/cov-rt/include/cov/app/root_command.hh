// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <cov/app/cov_tr.hh>
#include <cov/app/tools.hh>
#include <cov/app/args.hh>

using namespace std::literals;

namespace cov::app::root {
	class parser : public base_parser<covlng> {
	public:
		parser(::args::args_view const& arguments,
		       std::span<builtin_tool const> const& builtins,
		       str::translator_open_info const& langs);
		::args::args_view parse();
		void noent(std::string_view tool) const;

	private:
		std::span<builtin_tool const> builtins_;
	};

	tools setup_tools(std::span<builtin_tool const> const& builtins);
}  // namespace cov::app::root
