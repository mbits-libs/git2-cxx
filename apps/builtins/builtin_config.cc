// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/config.hh>
#include <cov/app/rt_path.hh>
#include <cov/git2/repository.hh>

namespace cov::app::builtin::config {
	using namespace app::config;

	namespace {
		std::error_code handle(git::config const& cfg,
		                       command cmd,
		                       std::vector<std::string> const& args) {
			switch (cmd) {
				case command::set_or_get:
					if (args.size() > 1) {
						return cfg.set_string(args[0].c_str(), args[1].c_str());
					}
					[[fallthrough]];
				case command::get: {
					auto value = cfg.get_string(args[0].c_str());
					if (value) {
						std::puts(value->c_str());
					}
					break;
				}  // GCOV_EXCL_LINE[WIN32]
				case command::get_all:
					cfg.get_multivar_foreach(args[0].c_str(), nullptr,
					                         [](git_config_entry const* entry) {
						                         puts(entry->value);
						                         return 0;
					                         });
					break;

				case command::add:
					return cfg.set_multivar(args[0].c_str(), "^$",
					                        args[1].c_str());

				case command::unset:
					return cfg.delete_entry(args[0].c_str());

				case command::unset_all:
					return cfg.delete_multivar(args[0].c_str(), ".*");

				case command::list:
					cfg.foreach_entry([](git_config_entry const* entry) {
						fputs(entry->name, stdout);
						fputc('=', stdout);
						fputs(entry->value, stdout);
						fputc('\n', stdout);
						return 0;
					});
					break;

				default:           // GCOV_EXCL_LINE
					[[unlikely]];  // GCOV_EXCL_LINE
					break;         // GCOV_EXCL_LINE
			}
			return {};
		}
	}  // namespace

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		p.parse();

		std::error_code ec{};
		auto const cfg =
		    open_config(p.which, platform::sys_root(), p.file, p.how, ec);
		if (!ec) ec = handle(cfg, p.cmd, p.args);
		if (ec) p.error(ec, p.tr());
		return 0;
	}  // GCOV_EXCL_LINE[WIN32]
}  // namespace cov::app::builtin::config
