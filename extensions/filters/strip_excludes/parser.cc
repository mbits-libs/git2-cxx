// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "parser.hh"
#include <concepts>

namespace cov::app::strip {
	template <typename E>
	void next(E& value) {
		value = static_cast<E>(std::to_underlying(value) + 1);
	}
	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<errlng, ExcludesLng>{langs, arguments} {
		using namespace str;

		parser_.arg(src_dir, "src")
		    .meta(tr_(args::lng::DIR_META))
		    .help(tr_(ExcludesLng::ARG_SRC))
		    .opt();
		parser_.arg(compiler, "compiler")
		    .meta(tr_(args::lng::NAME_META))
		    .help(fmt::format(fmt::runtime(tr_(ExcludesLng::ARG_COMPILER)),
		                      default_compiler));
		parser_.arg(os, "os")
		    .meta(tr_(args::lng::NAME_META))
		    .help(
		        fmt::format(fmt::runtime(tr_(ExcludesLng::ARG_OS)), platform));
		parser_.custom([&]() { next(verbose); }, "v")
		    .help(tr_(ExcludesLng::ARG_VERBOSE))
		    .opt();
	}

	void parser::parse() {
		parser_.parse();

		if (!compiler)
			compiler =
			    std::string{default_compiler.data(), default_compiler.size()};
		if (!os) os = std::string{platform.data(), platform.size()};

		os = tolower(trim(*os));
		compiler = tolower(trim(*compiler));
	}
}  // namespace cov::app::strip
