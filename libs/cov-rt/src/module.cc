// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/revparse.h>
#include <cov/app/module.hh>
#include <cov/app/path.hh>
#include <cov/app/rt_path.hh>
#include <cov/app/tools.hh>
#include <cov/repository.hh>

namespace cov::app::module {
	using namespace std::literals;

	namespace {
		struct cmd_info {
			std::pair<unsigned, unsigned> minmax;
			parser::args_description descr;
		};

		static constexpr cmd_info items[] = {
		    {{0, 1}, {modlng::COMMIT_META, modlng::SHOW_COMMIT_DESCRIPTION}},
		    {{2, 2},
		     {"--add"sv, modlng::ADD_DESCRIPTION, str::args::lng::NAME_META,
		      str::args::lng::DIR_META}},
		    {{2, 2},
		     {"--remove"sv, modlng::REMOVE_DESCRIPTION,
		      str::args::lng::NAME_META, str::args::lng::DIR_META}},
		    {{1, 1},
		     {"--remove-all"sv, modlng::REMOVA_ALL_DESCRIPTION,
		      str::args::lng::NAME_META}},
		    {{0, 1},
		     {"--show-sep"sv, modlng::SHOW_SEP_DESCRIPTION,
		      opt{modlng::COMMIT_META}}},
		    {{1, 1},
		     {"--set-sep"sv, modlng::SET_SEP_DESCRIPTION,
		      str::args::lng::VALUE_META}}};

		template <command Cmd>
		inline constexpr cmd_info const& info() {
			auto const index =
			    static_cast<std::underlying_type_t<command>>(Cmd);
			static_assert(index < std::size(items),
			              "Items and commands got out of sync.");
			return items[index];
		}

		template <command Cmd>
		inline constexpr parser::string name_of() noexcept {
			return info<Cmd>().descr.args;
		}
		inline constexpr parser::string name_of(command cmd) noexcept {
			auto const index =
			    static_cast<std::underlying_type_t<command>>(cmd);
			return items[index].descr.args;
		}
		inline constexpr std::pair<unsigned, unsigned> minmax(
		    command cmd) noexcept {
			auto const index =
			    static_cast<std::underlying_type_t<command>>(cmd);
			return items[index].minmax;
		}

		[[noreturn]] void show_help(::args::parser& p) {
			using str::args::lng;
			using enum command;

			auto prn = args::printer{stdout};
			auto& _ = base_parser<errlng, modlng>::tr(p);

			auto const _s = [&](auto arg) { return to_string(_(arg)); };

			auto object_meta = _(modlng::COMMIT_META);
			auto name_meta = _(lng::NAME_META);
			auto dir_meta = _(lng::DIR_META);
			auto value_meta = _(lng::VALUE_META);
			prn.format_paragraph(_s(lng::USAGE), 7);
			for (auto synopsis : {
			         " cov module [-h] [{0}]"sv,
			         " cov module [-h] --show-sep [{0}]"sv,
			         " cov module [-h] --set-sep {3}"sv,
			         " cov module [-h] --add {1} {2}"sv,
			         " cov module [-h] --remove {1} {2}"sv,
			         " cov module [-h] --remove-all {1}"sv,
			     }) {
				prn.format_paragraph(
				    fmt::format(fmt::runtime(synopsis), object_meta, name_meta,
				                dir_meta, value_meta),
				    7);
			}

			static const parser::args_description positionals[] = {
			    {modlng::NOARGS_META, modlng::SHOW_WORKDIR_DESCRIPTION},
			    info<show>().descr,
			};

			static const parser::args_description optionals[] = {
			    info<show_separator>().descr, info<set_separator>().descr,
			    info<add_path>().descr,       info<remove_path>().descr,
			    info<remove_module>().descr,
			};

			args::fmt_list args(2);
			parser::args_description::group(_, lng::POSITIONALS, positionals,
			                                args[0]);
			parser::args_description::group(_, lng::OPTIONALS, optionals,
			                                args[1]);

			prn.format_list(args);
			std::exit(0);
		}  // GCOV_EXCL_LINE

		ref_ptr<modules> make_empty() {
			return cov::modules::make_modules({}, {});
		}

	}  // namespace

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<errlng, modlng>{langs, arguments} {
		using namespace str;
		using enum command;

		parser_.usage(
		    fmt::format("[-h] [--show-sep [{0}]] | [--set-sep {1} | --add {2} "
		                "{3} | --remove {2} {3} | --remove-all {2}] | [{0}]"sv,
		                tr_(modlng::COMMIT_META), tr_(args::lng::VALUE_META),
		                tr_(args::lng::NAME_META), tr_(args::lng::DIR_META)));
		parser_.provide_help(false);
		parser_.custom(show_help, "h", "help").opt();
		parser_.custom(set_command<add_path>(), "add").opt();
		parser_.custom(set_command<remove_path>(), "remove").opt();
		parser_.custom(set_command<remove_module>(), "remove-all").opt();
		parser_.custom(set_command<set_separator>(), "set-sep").opt();
		parser_.custom([this] { handle_separator(); }, "show-sep").opt();
		parser_.custom([this](std::string const& val) { handle_arg(val); })
		    .opt();
	}

	void parser::handle_command(command arg, std::string const& val) {
		ops.push_back({arg, {val}});
	}

	void parser::handle_separator() {
		ops.push_back({command::show_separator});
	}

	void parser::handle_arg(std::string const& val) {
		if (ops.empty())
			ops.push_back({command::show, {val}});
		else
			ops.back().args.push_back(val);
	}

	void parser::parse() {
		using namespace str;

		parser_.parse();
		str_visitor visitor{tr_};

		if (ops.size() > 1) {
			for (auto const& op : ops) {
				if (op.cmd == command::show) {
					parser_.error(
					    tr_.format(modlng::ERROR_EXCLUSIVE, op.args.front()));
				}
				if (op.cmd == command::show_separator) {
					parser_.error(tr_.format(
					    modlng::ERROR_EXCLUSIVE,
					    visitor.visit(name_of<command::show_separator>())));
				}
			}
		}
		for (auto const& op : ops) {
			auto const [min, max] = minmax(op.cmd);
			if (min == max) {
				if (op.args.size() != max) {
					parser_.error(
					    fmt::format(fmt::runtime(tr_(
					                    modcnt::ERROR_OPTS_NEEDS_EXACTLY, max)),
					                visitor.visit(name_of(op.cmd)), max));
				}
			}
			if (op.args.size() > max) {
				parser_.error(fmt::format(
				    fmt::runtime(tr_(modcnt::ERROR_OPTS_NEEDS_AT_MOST, max)),
				    visitor.visit(name_of(op.cmd)), max));
			}
			if (min && op.args.size() < min) {
				// GCOV_EXCL_START -- currently only min != max ranges are
				// (0, 1), so this is not testable
				parser_.error(fmt::format(
				    fmt::runtime(tr_(modcnt::ERROR_OPTS_NEEDS_AT_LEAST, max)),
				    visitor.visit(name_of(op.cmd)), min));
				// GCOV_EXCL_STOP
			}
		}

		if (ops.empty()) ops.push_back({command::show});
	}

	git::repository parser::open_repo() const {
		std::error_code ec{};
		auto const cwd = std::filesystem::current_path(ec);
		if (ec) {
			[[unlikely]];    // GCOV_EXCL_LINE
			error(ec, tr_);  // GCOV_EXCL_LINE
		}
		auto const covdir = cov::repository::discover(cwd, ec);
		if (!covdir.empty()) {
			ec.clear();
			auto const repo =
			    cov::repository::open(platform::sys_root(), covdir, ec);
			if (!ec) {
				auto const gitdir = repo.config().get_path("core.gitdir");
				if (gitdir) {
					auto git_repo = git::repository::open(covdir / *gitdir, ec);
					if (git_repo && !ec) return git_repo;
				}  // GCOV_EXCL_LINE[WIN32]
			}      // GCOV_EXCL_LINE[WIN32]
		}          // GCOV_EXCL_LINE[WIN32]

		ec.clear();
		auto const gitdir = git::repository::discover(cwd, ec);
		if (!gitdir.empty()) {
			auto git_repo = git::repository::open(gitdir, ec);
			if (git_repo && !ec) return git_repo;
		}  // GCOV_EXCL_LINE[WIN32]

		ec.clear();
		auto git_repo = git::repository::open(cwd, ec);
		if (git_repo && !ec) return git_repo;

		error(tr_.format(str::args::lng::CANNOT_FIND_GIT, get_u8path(cwd)));
	}  // GCOV_EXCL_LINE[WIN32]

	ref_ptr<modules> parser::open_modules(
	    git::repository_handle repo,
	    std::vector<std::string> const& args) const {
		if (args.empty()) return open_from_workdir(repo);
		return open_from_ref(repo, args.front());
	}

	ref_ptr<modules> parser::open_from_workdir(
	    git::repository_handle repo) const {
		auto cfg = git::config::create();
		auto ec = cfg.add_file_ondisk(workdir_path(repo));
		if (ec) return make_empty();

		auto mods = cov::modules::from_config(cfg, ec);
		if (ec || !mods) {
			[[unlikely]];         // GCOV_EXCL_LINE
			mods = make_empty();  // GCOV_EXCL_LINE
		}
		return mods;
	}

	ref_ptr<modules> parser::open_from_ref(git::repository_handle repo,
	                                       std::string const& reference) const {
		git_revspec rs{};
		auto ec =
		    git::as_error(git_revparse(&rs, repo.get(), reference.c_str()));
		if (ec) return make_empty();

		git::ptr<git_object> from{rs.from};
		git::ptr<git_object> to{rs.to};

		if ((rs.flags & GIT_REVSPEC_SINGLE) != GIT_REVSPEC_SINGLE)
			error(to_string(tr_(modlng::ERROR_REVPARSE_NEEDS_SINGLE)));

		auto const obj_type = git_object_type(rs.from);
		if (obj_type != GIT_OBJECT_COMMIT) {
			error(to_string(tr_(modlng::ERROR_REVPARSE_NEEDS_COMMIT)));
		}

		auto mods =
		    cov::modules::from_commit(*git_object_id(rs.from), repo, ec);
		if (ec || !mods) {
			[[unlikely]];         // GCOV_EXCL_LINE
			mods = make_empty();  // GCOV_EXCL_LINE
		}
		return mods;
	}

	std::filesystem::path parser::workdir_path(
	    git::repository_handle repo) const {
		auto const workdir = repo.workdir();
		if (!workdir) error(to_string(tr_(modlng::ERROR_NO_GIT_WORKDIR)));
		return make_u8path(*workdir) / ".covmodule"sv;
	}
}  // namespace cov::app::module
