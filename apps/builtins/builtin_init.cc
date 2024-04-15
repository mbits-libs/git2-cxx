// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/app/args.hh>
#include <cov/app/cov_init_tr.hh>
#include <cov/app/errors_tr.hh>
#include <cov/app/path.hh>
#include <cov/app/rt_path.hh>
#include <cov/git2/repository.hh>
#include <cov/repository.hh>

namespace cov::app::builtin::init {
	namespace {
		using namespace std::filesystem;
		using namespace std::literals;

		bool starts_with(path const& canonical_inner,
		                 path const& canonical_outer) {
			using path_view = std::basic_string_view<path::value_type>;
			auto const outer = path_view{canonical_outer.native()};
			auto const inner = path_view{canonical_inner.native()};
			return inner.starts_with(outer);
		}

		path git_root(path const& common) {
			std::error_code ec;
			auto const repo = git::repository::open(common, ec);
			if (!repo) {
				[[unlikely]];   // GCOV_EXCL_LINE
				return common;  // GCOV_EXCL_LINE
			}  // GCOV_EXCL_LINE

			auto const workdir = repo.work_dir();
			if (workdir) {
				ec.clear();
				auto result = weakly_canonical(make_u8path(*workdir), ec);
				if (ec) {
					[[unlikely]];   // GCOV_EXCL_LINE
					return common;  // GCOV_EXCL_LINE
				}  // GCOV_EXCL_LINE
				return result;
			}

			return common;
		}
	}  // namespace

	struct parser : base_parser<errlng, initlng> {
		struct arguments {
			path git_dir{};
			path directory{};
			std::string branch{};
			int flags;
		};

		parser(::args::args_view const& arguments,
		       str::translator_open_info const& langs);

		arguments parse();

	private:
		std::optional<std::string> git_dir_{};
		std::optional<std::string> directory_{};
		std::optional<std::string> worktree_branch_{};
		bool force_{};
	};

	parser::parser(::args::args_view const& arguments,
	               str::translator_open_info const& langs)
	    : base_parser<errlng, initlng>{langs, arguments} {
		using namespace str;

		parser_.arg(git_dir_, "git-dir")
		    .meta(tr_(args::lng::DIR_META))
		    .help(tr_(cov_init::lng::ARG_GITDIR));
		parser_.set<std::true_type>(force_, "force")
		    .help(tr_(cov_init::lng::ARG_FORCE))
		    .opt();
		parser_.arg(worktree_branch_, "worktree")
		    .meta(tr_(cov_init::lng::BRANCH_META))
		    .help(tr_(cov_init::lng::ARG_WORKTREE));
		parser_.arg(directory_)
		    .meta(tr_(cov_init::lng::DIR_META))
		    .help(tr_(cov_init::lng::ARG_DIRECTORY));
	}

	parser::arguments parser::parse() {
		using namespace str;

		parser_.parse();

		arguments result = {
		    .flags = (force_ ? cov::init_options::reinit : 0) |
		             (worktree_branch_ ? cov::init_options::worktree : 0),
		};
		bool had_both = git_dir_ && directory_;

		std::error_code ec{};
		if (worktree_branch_) {
			result.branch = std::move(*worktree_branch_);
		}

		if (!git_dir_) {
			if (directory_) {
				git_dir_ = directory_;
			} else {
				auto const cwd = current_path(ec);
				// GCOV_EXCL_START[WIN32]
				if (ec) {
					[[unlikely]];
					error(tr_.format(args::lng::ARGUMENT_MSG, "-C"sv,
					                 platform::con_to_u8(ec)));
				}
				// GCOV_EXCL_STOP
				git_dir_ = get_u8path(cwd);
			}
		}

		result.git_dir = git::repository::discover(make_u8path(*git_dir_), ec);
		if (ec) {
			std::error_code ec2;
			auto const arg_git_dir =
			    get_u8path(weakly_canonical(make_u8path(*git_dir_), ec2));
			// GCOV_EXCL_START
			if (ec2) {
				[[unlikely]];
				error(tr_.format(args::lng::ARGUMENT_MSG, "--git-dir"sv,
				                 platform::con_to_u8(ec2)));
			}
			// GCOV_EXCL_STOP

			error(tr_.format(cov_init::lng::NOT_GIT, arg_git_dir));
		}  // GCOV_EXCL_LINE

		result.directory = result.git_dir / ".covdata"sv;

		if (had_both) {
			auto canonical = weakly_canonical(make_u8path(*directory_), ec);
			// GCOV_EXCL_START
			if (ec) {
				[[unlikely]];
				error(tr_.format(args::lng::ARGUMENT_MSG, "directory"sv,
				                 platform::con_to_u8(ec)));
			}
			// GCOV_EXCL_STOP

			auto const root_dir = git_root(result.git_dir);

			if (!starts_with(canonical / ""sv, root_dir / ""sv)) {
				result.directory = std::move(canonical);
			}
		}

		return result;
	}  // GCOV_EXCL_LINE

	int handle(std::string_view tool, args::arglist args) {
		using namespace str;
		parser p{{tool, args},
		         {platform::locale_dir(), ::lngs::system_locales()}};
		auto const [git_dir, directory, branch, flags] = p.parse();

		std::error_code ec{};
		auto const repo =
		    cov::repository::init(platform::sys_root(), directory, git_dir, ec,
		                          {.flags = flags, .branch_name = branch});
		if (!ec) {
			p.tr().print(flags & cov::init_options::reinit
			                 ? cov_init::lng::REINITIALIZED
			                 : cov_init::lng::INITIALIZED,
			             get_u8path(repo.cov_dir()));
			fputs("\n ", stdout);
			p.tr().print(
			    cov_init::lng::USING_GIT,
			    get_u8path(weakly_canonical(make_u8path(repo.git_dir()))));
			fputc('\n', stdout);
			return 0;
		}

		if (ec == std::errc::file_exists) {
			p.tr().print(cov_init::lng::EXISTS, get_u8path(directory));
			std::fputc('\n', stdout);
			return 1;
		}

		if (ec == cov::errc::not_a_worktree) {
			p.tr().print(cov_init::lng::NOT_WORKTREE, get_u8path(git_dir));
			std::fputc('\n', stdout);
			return 1;
		}

		[[unlikely]];
		p.tr().print(cov_init::lng::CANNOT_INITIALIZE, get_u8path(directory));
		std::fputc('\n', stdout);
		p.error(ec, p.tr());
	}  // GCOV_EXCL_LINE[WIN32]
}  // namespace cov::app::builtin::init
