// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/strings/args.hh>
#include <cov/app/tr.hh>
#include <args/parser.hpp>

namespace cov::app {
	template <typename... Enum>
	requires(std::is_enum_v<Enum>&&...) class base_parser {
	public:
		base_parser(str::translator_open_info const& langs,
		            ::args::args_view const& arguments,
		            std::string const& description = {})
		    : tr_{langs}, parser_{description, arguments, tr_.args()} {}

		void parse() { parser_.parse(); }

		static str::args_translator<Enum...> const& tr(
		    ::args::parser const& p) noexcept {
			return str::args_translator<Enum...>::from(p.tr());
		}
		str::args_translator<Enum...> const& tr() const noexcept { return tr_; }

		[[noreturn]] void error(std::string const& msg) const {
			parser_.error(msg);
		}

	protected:
		str::args_translator<Enum...> tr_;
		::args::parser parser_;
	};
}  // namespace cov::app
