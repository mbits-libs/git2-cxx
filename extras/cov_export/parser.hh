// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/parser.hpp>
#include <cov/app/args.hh>
#include <cov/app/strings/cov.hh>
#include <cov/app/strings/errors.hh>
#include <cov/app/tr.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>
#include <native/open_here.hh>
#include <native/platform.hh>

namespace cov::app {
	using covlng = str::root::lng;
	using CovStrings = str::root::Strings;

	template <>
	struct lngs_traits<covlng> : base_lngs_traits<covlng, "cov", CovStrings> {};
}  // namespace cov::app

namespace cov::app {
	using errlng = str::errors::lng;
	using ErrorsStrings = str::errors::Strings;

	template <>
	struct lngs_traits<errlng>
	    : base_lngs_traits<errlng, "errors", ErrorsStrings> {};
}  // namespace cov::app

namespace cov::app::report_export {
	enum class op { none, html, json };

	struct parser : base_parser<covlng, errlng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		struct response {
			bool only_json;
			revs range{};
			std::string path{};
			cov::repository repo{};
			unsigned verbose{};
		};

		response parse();

		cov::repository open_here() const {
			return cov::app::open_here(platform::core_extensions::sys_root(),
			                           *this, tr_);
		}

		op oper{op::none};
		std::optional<std::string> rev{};
		std::string path{};
		unsigned verbose{};
	};
}  // namespace cov::app::report_export
