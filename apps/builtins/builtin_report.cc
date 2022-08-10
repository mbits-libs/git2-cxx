// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <cov/app/args.hh>
#include <cov/app/cov_report_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/path.hh>
#include <cov/app/report.hh>
#include <cov/app/report_command.hh>
#include <cov/app/rt_path.hh>
#include <cov/io/file.hh>

namespace cov::app::builtin::report {
	using namespace app::report;
	using namespace std::filesystem;
	using namespace std::literals;

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		auto [repo, report] = p.parse();

		std::error_code ec{};
		auto const commit = git_commit::load(repo.git(), report.git.head, ec);
		if (ec) {
			p.data_error(replng::ERROR_CANNOT_LOAD_COMMIT);
		}
		auto files = stored_file::from(commit, report.files, p);

		auto it = files.begin();
		for (auto const& file : report.files) {
			auto& compiled = *it++;
			compiled.store(repo, file, p);
		}

		git_oid file_coverage{};
		if (!stored_file::store_tree(file_coverage, repo, report.files,
		                             files)) {
			// GCOV_EXCL_START
			[[unlikely]];
			p.data_error(replng::ERROR_CANNOT_WRITE_TO_DB);
			// GCOV_EXCL_STOP
		}

		auto coverage = io::v1::coverage_stats::init();
		for (auto const& file : files) {
			coverage += file.stats;
		}

		auto const [branch, current_id] = p.update_current_branch(
		    repo, file_coverage, report.git, commit, coverage);

		auto const resulting = repo.lookup<cov::report>(current_id, ec);
		if (!resulting || ec) {
			// GCOV_EXCL_START
			[[unlikely]];
			p.data_error(replng::ERROR_CANNOT_READ_FROM_DB);
			// GCOV_EXCL_STOP
		}

		p.print_report(branch, files.size(), *resulting);

		return 0;
	}
}  // namespace cov::app::builtin::report
