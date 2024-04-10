// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/args.hh>
#include <cov/app/dirs.hh>
#include <cov/app/path.hh>
#include <cov/app/rt_path.hh>
#include <cov/repository.hh>

namespace cov::app {
	cov::repository open_here(parser_holder const& parser,
	                          str::errors::Strings const& err_str,
	                          str::args::Strings const& arg_str) {
		std::error_code ec{};
		auto const current_directory = std::filesystem::current_path(ec);
		// GCOV_EXCL_START
		if (ec) {
			[[unlikely]];
			parser.error(ec, err_str, arg_str);
		}
		// GCOV_EXCL_STOP

		auto const repo_path = cov::discover_repository(current_directory, ec);
		if (ec) {
			if (ec == errc::uninitialized_worktree) {
				fmt::print(stderr, "{}\n\n    cov init --worktree <branch>\n\n",
				           arg_str(str::args::lng::COV_WITHIN_WORKTREE));
				std::exit(1);
			}

			parser.error(fmt::format(
			    fmt::runtime(arg_str(str::args::lng::CANNOT_FIND_COV)),
			    get_u8path(current_directory)));
		}

		auto repo = cov::repository::open(platform::sys_root(), repo_path, ec);
		if (ec) parser.error(ec, err_str, arg_str);

		return repo;
	}
}  // namespace cov::app

namespace cov::app::platform {
	std::filesystem::path const& sys_root() {
		// dirname / ..
		static auto const root = exec_path().parent_path().parent_path();
		return root;
	}

	std::filesystem::path locale_dir() {
		return sys_root() / directory_info::share / "locale"sv;
	}  // GCOV_EXCL_LINE
}  // namespace cov::app::platform
