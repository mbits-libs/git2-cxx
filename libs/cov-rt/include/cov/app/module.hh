// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/args.hh>
#include <cov/app/cov_module_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/module.hh>
#include <git2/repository.hh>
#include <string>
#include <vector>

namespace cov::app::module {
	enum class command {
		show,
		add_path,
		remove_path,
		remove_module,
		show_separator,
		set_separator
	};

	struct operation {
		command cmd;
		std::vector<std::string> args{};
	};

	struct parser : base_parser<errlng, modlng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		void parse();

		void handle_command(command arg, std::string const& val);
		void handle_separator();
		void handle_arg(std::string const& val);

		git::repository open_repo() const;
		ref_ptr<modules> open_modules(
		    git::repository_handle repo,
		    std::vector<std::string> const& args) const;
		ref_ptr<modules> open_from_workdir(git::repository_handle repo) const;
		ref_ptr<modules> open_from_ref(git::repository_handle repo,
		                               std::string const& reference) const;
		std::filesystem::path workdir_path(git::repository_handle repo) const;

		template <command Arg>
		auto set_command() {
			return
			    [this](std::string const& name) { handle_command(Arg, name); };
		}

		std::vector<operation> ops{};
	};
}  // namespace cov::app::module
