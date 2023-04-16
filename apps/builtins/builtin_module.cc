// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/app/module.hh>

#include <fmt/format.h>
#include <cov/app/args.hh>
#include <cov/app/cov_module_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/rt_path.hh>

namespace cov::app::builtin::module {
	using namespace app::module;
	using namespace std::filesystem;
	using namespace std::literals;

	namespace {
		void show_modules(modules const& mods) {
			for (auto const& entry : mods.entries()) {
				std::puts(entry.name.c_str());
				for (auto const& prefix : entry.prefixes) {
					std::fputs("- ", stdout);
					std::puts(prefix.c_str());
				}
			}
		}

		void show_separator(modules const& mods) {
			auto const sep = mods.separator();
			if (!sep.empty()) fmt::print("{}\n", sep);
		}
	}  // namespace

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		p.parse();
		git::repository git_repo = p.open_repo();
		ref_ptr<modules> mods;

		bool modified = false;
		mod_errc result{};
		for (auto const& op : p.ops) {
			switch (op.cmd) {
				case command::show:
					show_modules(*p.open_modules(git_repo, op.args));
					return 0;

				case command::show_separator:
					show_separator(*p.open_modules(git_repo, op.args));
					return 0;

				case command::set_separator:
					if (!mods) mods = p.open_from_workdir(git_repo);
					result = mods->set_separator(op.args.front());
					break;
				case command::add_path:
					if (!mods) mods = p.open_from_workdir(git_repo);
					result = mods->add(op.args[0], op.args[1]);
					break;
				case command::remove_path:
					if (!mods) mods = p.open_from_workdir(git_repo);
					result = mods->remove(op.args[0], op.args[1]);
					break;
				case command::remove_module:
					if (!mods) mods = p.open_from_workdir(git_repo);
					result = mods->remove_all(op.args.front());
					break;
			}

			if (result == mod_errc::unmodified) continue;
			if (result == mod_errc::needs_update) {
				modified = true;
				continue;
			}
			if (result == mod_errc::duplicate)
				p.error(p.tr().format(modlng::ERROR_ADD_DUPLICATE, op.args[0],
				                      op.args[1]));
			if (result == mod_errc::no_module)
				p.error(
				    p.tr().format(modlng::ERROR_REMOVE_NO_MODULE, op.args[0]));
		}  // GCOV_EXCL_LINE

		if (modified) {
			auto const cfg_path = p.workdir_path(git_repo);
			auto cfg = git::config::create();
			auto ec = cfg.add_file_ondisk(cfg_path);
			if (ec) {
				if (ec == git::errc::notfound)
					ec = make_error_code(git::errc::file_not_found);
				p.error(ec, p.tr());
			}
			mods->dump(cfg);
			modules::cleanup_config(cfg_path);
		}
		return 0;
	}
}  // namespace cov::app::builtin::module
