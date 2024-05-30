// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "parser.hh"

using namespace std::literals;

namespace cov::app::report_export {
	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<covlng, errlng>{langs, arguments} {
		using namespace str;

		parser_.usage(fmt::format(
		    "cov export [-h] [{}] (--json {} | --html {})",
		    tr_(covlng::REPORT_META), tr_(str::args::lng::FILE_META),
		    tr_(str::args::lng::DIR_META)));
		parser_
		    .arg(rev)  // GCOV_EXCL_LINE[GCC]
		    .meta(tr_(covlng::REPORT_META))
		    .help(
		        "shows either changes between given report and the report "
		        "directly preceeding (if only one reference is given), or "
		        "between two reports (when there are two refs separated by a "
		        "'..'); defaults to HEAD if missing");
		parser_
		    .custom(
		        [&](std::string_view arg) {
			        path.assign(arg);
			        oper = op::json;
		        },
		        "json")
		    .meta(tr_(str::args::lng::FILE_META))
		    .opt();
		parser_
		    .custom(
		        [&](std::string_view arg) {
			        path.assign(arg);
			        oper = op::html;
		        },
		        "html")
		    .meta(tr_(str::args::lng::DIR_META))
		    .opt();
		parser_.custom([&]() { ++verbose; }, "v").opt();
	}

	parser::response parser::parse() {
		using namespace str;

		parser_.parse();

		if (oper == op::none) {
			parser_.error("format (either --html or --json) is required");
		}

		response result{};

		result.repo = open_here();

		if (!rev) rev = "HEAD"s;
		result.path = path;
		result.only_json = oper == op::json;
		result.verbose = verbose;

		auto ec = revs::parse(result.repo, *rev, result.range);
		if (ec) error(ec, tr_);

		return result;
	}  // GCOV_EXCL_LINE[GCC]

}  // namespace cov::app::report_export
