// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/args.hh>
#include <cov/app/cov_checkout_tr.hh>
#include <cov/app/cov_module_tr.hh>
#include <cov/app/cov_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/git2/repository.hh>
#include <string>
#include <vector>

namespace cov {
	struct repository;
}

namespace cov::app::checkout {
	enum class command {
		unspecified,
		checkout,
		detach,
		branch_and_checkout,
		checkout_orphaned,
	};

	struct parser : base_parser<covlng, errlng, modlng, co_lng> {
		using base = base_parser<covlng, errlng, modlng, co_lng>;
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		void parse();
		cov::repository open_here() const;
		std::error_code run(cov::repository const&) const;

		template <command Arg>
		auto set_command() {
			return [this]() { handle_command(Arg); };
		}

	private:
		void handle_command(command arg);
		void handle_arg(std::string const& val);

		std::vector<std::string> names_{};
		command cmd_{command::unspecified};
	};
}  // namespace cov::app::checkout
