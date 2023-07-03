// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <args/actions.hpp>
#include <cov/app/args.hh>
#include <cov/app/cov_reset_tr.hh>
#include <cov/app/cov_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/path.hh>
#include <cov/app/report_command.hh>
#include <cov/app/rt_path.hh>
#include <cov/app/show_range.hh>
#include <cov/repository.hh>
#include <cov/revparse.hh>
#include <git2/repository.hh>

namespace cov::app::builtin::reset {
	struct parser : base_parser<errlng, covlng, replng, resetlng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		cov::repository open_here() const {
			return cov::app::open_here(*this, tr_);
		}

		std::string report_id;
	};

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser{langs, arguments} {
		using namespace str;

		parser_.arg(report_id)
		    .meta(tr_(covlng::REPORT_META))
		    .help(tr_(resetlng::REPORT_DESCRIPTION));
	}

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		p.parse();
		auto const repo = p.open_here();

		git_oid new_tip;
		auto ec = revs::parse_single(repo, p.report_id, new_tip);
		if (ec) p.error(ec, p.tr());

		auto const head = repo.current_head();
		if (repo.update_current_head(new_tip, head)) {
			char buffer[GIT_OID_HEXSZ + 1];
			git_oid_fmt(buffer, &new_tip);
			buffer[GIT_OID_HEXSZ] = 0;

			auto const msg =
			    head.branch.empty()
			        ? fmt::format(
			              fmt::runtime(p.tr()(resetlng::DETACHED_TIP_MOVED_TO)),
			              buffer)
			        : fmt::format(
			              fmt::runtime(p.tr()(resetlng::BRANCH_TIP_MOVED_TO)),
			              head.branch, buffer);

			fmt::print("{}\n", msg);

			auto const report = repo.lookup<cov::report>(new_tip, ec);
			if (!ec && report) {
				auto const files =
				    repo.lookup<cov::report_files>(report->file_list(), ec);
				auto const file_count =
				    files && !ec ? files->entries().size() : 0u;
				builtin::report::parser::print_report(head.branch, file_count,
				                                      *report, p.tr());
			}
		}

		return 0;
	}
}  // namespace cov::app::builtin::reset
