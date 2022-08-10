// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/args.hh>
#include <cov/app/cov_log_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/path.hh>
#include <cov/app/rt_path.hh>
#include <cov/app/show_range.hh>
#include <cov/format.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>
#include <git2/repository.hh>

namespace cov::app::builtin::log {
	struct parser : base_parser<loglng, errlng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		struct response {
			revs range{};
			cov::repository repo{};
		};

		response parse();

		cov::repository open_here() const {
			return cov::app::open_here(*this, tr_);
		}

		std::optional<std::string> rev{};
		std::optional<unsigned> count{};
		show_range show{};
	};

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<loglng, errlng>{langs, arguments} {
		using namespace str;

		parser_.arg(rev)
		    .meta(tr_(loglng::REV_RANGE_META))
		    .help(tr_(loglng::REV_RANGE_DESCRIPTION));
		parser_.arg(count, "n", "max-count")
		    .meta(tr_(loglng::NUMBER_META))
		    .help(tr_(loglng::MAX_COUNT_DESCRIPTION));
		show.add_args(parser_, tr_);
	}

	parser::response parser::parse() {
		using namespace str;

		parser_.parse();

		response result{};

		result.repo = open_here();

		if (!rev) rev = "HEAD"sv;
		auto ec = revs::parse(result.repo, *rev, result.range);
		if (ec) error(ec, tr_);

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		auto const info = p.parse();

		p.show.print(info.repo, info.range, p.count);
		return 0;
	}
}  // namespace cov::app::builtin::log
