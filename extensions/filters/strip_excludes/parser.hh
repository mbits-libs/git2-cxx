// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/args.hh>
#include <cov/app/strings/cov.hh>
#include <cov/app/strings/errors.hh>
#include <cov/app/tr.hh>
#include <native/str.hh>
#include <optional>
#include <string>
#include <string_view>
#include "excludes_tr.hh"

using namespace std::literals;

namespace cov::app {
	using errlng = str::errors::lng;
	using ErrorsStrings = str::errors::Strings;

	template <>
	struct lngs_traits<errlng>
	    : base_lngs_traits<errlng, "errors", ErrorsStrings> {};
};  // namespace cov::app

namespace cov::app::strip {
#ifdef _WIN32
	static constexpr auto default_compiler = "msvc"sv;
	static constexpr auto platform = "win32"sv;
#endif
#ifdef __linux__
	static constexpr auto default_compiler = "gcc"sv;
	static constexpr auto platform = "posix"sv;
#endif

	enum detail {
		none = 0,
		stats = 1,
		listing = 2,
	};

	struct parser : base_parser<errlng, ExcludesLng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		void parse();

		std::string src_dir{".", 1};
		std::optional<std::string> compiler;
		std::optional<std::string> os;
		detail verbose{detail::none};
	};
}  // namespace cov::app::strip
