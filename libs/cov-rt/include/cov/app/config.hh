// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/app/args.hh>
#include <cov/app/cov_config_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/git2/config.hh>
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

namespace cov::app::config {
	enum class scope { unspecified, local, global, system, file };
	enum class mode { read, write };
	enum class command {
		unspecified,
		set_or_get,
		add,
		get,
		get_all,
		unset,
		unset_all,
		list
	};

	struct parser : base_parser<errlng, cfglng> {
		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		void parse();

		void handle_command(command arg, std::string const& val);
		void handle_list();
		void handle_arg(std::string const& val);

		template <command Arg>
		auto set_command() {
			return
			    [this](std::string const& name) { handle_command(Arg, name); };
		}

		scope which{scope::unspecified};
		mode how{mode::read};
		command cmd{command::unspecified};

		std::filesystem::path file{};
		std::vector<std::string> args{};
	};

	git::config open_config(scope which,
	                        std::filesystem::path const& sysroot,
	                        std::filesystem::path const& path,
	                        mode how,
	                        std::error_code& ec);
}  // namespace cov::app::config
