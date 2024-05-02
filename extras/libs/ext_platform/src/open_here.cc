// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/args.hh>
#include <cov/app/dirs.hh>
#include <cov/app/path.hh>
#include <cov/repository.hh>
#include <native/open_here.hh>

namespace cov::app {
	cov::repository open_here(std::filesystem::path const& sys_root,
	                          parser_holder const& parser,
	                          str::errors::Strings const& err_str,
	                          str::args::Strings const& arg_str) {
		std::error_code ec{};
		auto const repo_path = parser.locate_repo_here(err_str, arg_str);
		auto repo = cov::repository::open(sys_root, repo_path, ec);
		if (ec) parser.error(ec, err_str, arg_str);

		return repo;
	}
}  // namespace cov::app
