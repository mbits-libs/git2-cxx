// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/args.hh>
#include <cov/app/branches.hh>
#include <cov/app/log_format_tr.hh>
#include <cov/app/path.hh>
#include <cov/app/rt_path.hh>
#include <cov/format.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>
#include <git2/repository.hh>

namespace cov::app::builtin::branch {
	using base_parser = app::branches_base_parser;
	struct parser : base_parser {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		void parse();

		cov::repository open_here() const {
			return cov::app::open_here(*this, tr_);
		}

		app::params params{.tgt = target::branch};
	};

	namespace {
		[[noreturn]] void show_help(::args::parser& p) {
			using str::args::lng;

			static constexpr parser::args_description positionals[] = {
			    {lng::NAME_META, refslng::BRANCH_NAME_DESCRIPTION},
			};

			static constexpr parser::args_description optionals[] = {
			    {"-h, --help"sv, str::args::lng::HELP_DESCRIPTION},
			    {"-f, --force"sv, refslng::BRANCH_FORCE_DESCRIPTION},
			    {"-l, --list"sv, refslng::BRANCH_LIST_DESCRIPTION,
			     opt_multi{refslng::PATTERN_META}},
			    {"-d, --delete"sv, refslng::BRANCH_DELETE_DESCRIPTION,
			     multi{lng::NAME_META}},
			    {"--show-current"sv, refslng::BRANCH_SHOW_CURRENT_DESCRIPTION},
			    {"-q, --quiet"sv, refslng::BRANCH_QUIET_DESCRIPTION},
			    {"--color"sv, loglng::WHEN_DESCRIPTION, loglng::WHEN_META},
			};

			p.short_help();

			args::fmt_list args(2);
			auto& _ = parser::tr(p);
			parser::args_description::group(_, lng::POSITIONALS, positionals,
			                                args[0]);
			parser::args_description::group(_, lng::OPTIONALS, optionals,
			                                args[1]);

			auto prn = args::printer{stdout};
			prn.format_list(args);
			std::exit(0);
		}
	}  // namespace

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser{langs, arguments} {
		using namespace str;
		using enum command;

		parser_.usage(fmt::format(
		    "[-h] [{0} | -d {0}... | -l [{1}...] | --show-current]"sv,
		    tr_(args::lng::NAME_META), tr_(refslng::PATTERN_META)));
		parser_.provide_help(false);
		parser_.custom(show_help, "h", "help").opt();
		params.setup(parser_, *this);
	}

	void parser::parse() {
		using enum command;

		parser_.parse();
		params.validate_using(*this);
	}

	int handle(std::string_view tool, args::arglist args) {
		using enum command;
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		p.parse();
		auto const repo = p.open_here();

		auto const ec = p.params.run(repo);
		if (ec) {
			auto const error = [tool, &p](std::string_view message) {
				simple_error(p.tr(), tool, message);
			};

			if (ec == cov::errc::current_branch) {
				auto fmt = p.tr()(str::errors::lng::BRANCH_NOT_DELETABLE);
				error(fmt::format(fmt::runtime(fmt), p.params.names.front(),
				                  get_u8path(repo.commondir())));
			}  // GCOV_EXCL_LINE

			if (ec == git::errc::exists) {
				auto fmt = p.tr()(str::errors::lng::BRANCH_EXISTS);
				error(fmt::format(fmt::runtime(fmt), p.params.names.front()));
			}  // GCOV_EXCL_LINE

			if (ec == git::errc::invalidspec) {
				auto fmt = p.tr()(str::errors::lng::BRANCH_NAME_INVALID);
				error(fmt::format(fmt::runtime(fmt), p.params.names.front()));
			}  // GCOV_EXCL_LINE

			p.error(ec, p.tr());
		}

		return 0;
	}
}  // namespace cov::app::builtin::branch
