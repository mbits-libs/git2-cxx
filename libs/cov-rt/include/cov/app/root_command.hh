// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <cov/app/cov_tr.hh>
#include <cov/app/tools.hh>

using namespace std::literals;

namespace cov::app::root {
	class parser {
	public:
		parser(::args::args_view const& arguments,
		       std::span<builtin_tool const> const& builtins,
		       std::filesystem::path const& locale_dir,
		       std::span<std::string const> const& langs);
		::args::args_view parse();
		void noent(std::string_view tool) const;

		static str::args_translator<covlng> const& tr(args::parser const& p) noexcept {
			return str::args_translator<covlng>::from(p.tr());
		}
		str::args_translator<covlng> const& tr() const noexcept { return tr_; }

	private:
		::args::parser setup_parser(::args::args_view const& arguments,
		                            std::filesystem::path const& locale_dir,
		                            std::span<std::string const> const& langs);
		str::args_translator<covlng> tr_{};
		::args::parser parser_;
		std::span<builtin_tool const> builtins_;
	};

	tools setup_tools(std::span<builtin_tool const> const& builtins);
}  // namespace cov::app::root
