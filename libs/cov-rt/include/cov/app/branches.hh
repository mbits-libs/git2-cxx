// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/args.hh>
#include <cov/app/cov_log_tr.hh>
#include <cov/app/cov_module_tr.hh>
#include <cov/app/cov_refs_tr.hh>
#include <cov/app/cov_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/format.hh>
#include <cov/format_args.hh>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace cov::app::platform {
	bool glob_matches(std::filesystem::path const& match,
	                  std::filesystem::path const name);
}  // namespace cov::app::platform

namespace cov::app {
	enum class command { unspecified, list, create, remove, current };
	enum class target : bool { tag = false, branch = true };

	using branches_base_parser =
	    base_parser<errlng, covlng, modlng, loglng, refslng>;

	struct params {
		target tgt;
		std::vector<std::string> names{};
		command cmd{command::unspecified};
		bool force{false};
		bool quiet{false};
		color_feature color{};

		void setup(::args::parser& parser, branches_base_parser& cli);
		void validate_using(branches_base_parser& cli);
		std::error_code run(cov::repository const&) const;

	private:
		template <command Arg>
		auto set_command(branches_base_parser& cli) {
			return [this, &cli]() { set_command(cli, Arg); };
		}
		void set_command(branches_base_parser& cli, command cmd);

		auto add_name() {
			return [this](std::string const& name) {
				if (cmd == command::unspecified) cmd = command::create;
				names.push_back(name);
			};
		}

		void list(cov::repository const&) const;
		std::error_code create(cov::repository const&) const;
		std::error_code remove(cov::repository const&) const;
		void show_current(cov::repository const&) const;
	};
}  // namespace cov::app
