// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "parser.hh"

namespace cov::app::strip {
	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<errlng>{langs, arguments} {
		using namespace str;

		parser_.arg(src_dir, "src").meta(tr_(args::lng::DIR_META)).opt();
		parser_.arg(compiler, "c", "compiler").meta(tr_(args::lng::NAME_META));
		parser_.arg(os, "os").meta(tr_(args::lng::NAME_META));
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
