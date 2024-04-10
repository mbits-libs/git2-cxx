// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/discover.hh>
#include <cov/git2/config.hh>
#include <cov/git2/error.hh>
#include <cov/git2/repository.hh>
#include <cov/init.hh>
#include <cov/io/file.hh>
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <cov/repository.hh>
#include <sstream>
#include "path-utils.hh"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace cov {
	namespace {
		struct init_transaction {
			path root{};
			bool committed{false};

			void commit() { committed = true; }
			~init_transaction() {
				if (committed) return;

				std::error_code ec;
				remove_all(root, ec);
			}
		};

		std::error_code init_config(path const& cov_dir, path const& git_dir) {
			std::error_code ec{};
			auto cfg = git::config::create(ec);
			if (!cfg) return ec;

			if ((ec = cfg.add_file_ondisk(
			         get_path(cov_dir / names::config).c_str()))) {
				// GCOV_EXCL_START -- untestable, git_add_file_ondisk returns
				// error, if the stat failed, not due to ENOENT/ENOTDIR;
				// however, this would require messing with covdata directory,
				// which would be detected by create_directories below
				return ec;
			}  // GCOV_EXCL_STOP

			auto const gitdir = rel_path(git_dir, cov_dir);

			auto out = io::fopen(cov_dir / names::gitdir_link, "wb");
			if (out) {
				auto const text = fmt::format("{}\n", get_path(gitdir));
				out.store(text.data(), text.size());
			}

			return cfg.set_path(names::core_gitdir, gitdir);
		}

		std::error_code touch(path const& filename, std::string_view contents) {
			auto const out = io::fopen(filename, "wb");

			if (!out) {
				auto const error = errno;
				return std::make_error_code(error
				                                ? static_cast<std::errc>(error)
				                                : std::errc::permission_denied);
			}

			out.store(contents.data(), contents.size());
			return {};
		}

		struct linked_repository {
			path cov_dir{};
			std::string commondir{};
			std::string covdir{};
			std::string gitdir{};
			std::string HEAD{};
		};

		linked_repository prepare_worktree(path const& cov_dir,
		                                   path const& git_dir,
		                                   std::error_code& ec,
		                                   std::string_view branch_name) {
			linked_repository result{};

			auto const git_worktree = git::repository::open(git_dir, ec);
			if (ec) return result;

			if (!git_worktree.is_worktree()) {
				ec = make_error_code(cov::errc::not_a_worktree);
				return result;
			}

			auto const common_dir = git_worktree.common_dir();
			if (common_dir.empty()) {
				// GCOV_EXCL_START
				ec = make_error_code(git::errc::notfound);
				return result;
				// GCOV_EXCL_STOP
			}

			auto const cov_path =
			    discover_repository(make_path(common_dir), ec);
			if (ec) return result;

			auto const cov_repository = cov::repository::open({}, cov_path, ec);
			if (ec) return result;

			auto const ref = cov_repository.refs()->dwim(branch_name);
			if (!ref) {
				ec = make_error_code(git::errc::notfound);
				return result;
			}

			if (!ref->references_branch()) {
				ec = make_error_code(cov::errc::not_a_branch);
				return result;
			}

			auto dirname = git_dir;
			if (dirname.filename().empty()) dirname = dirname.parent_path();
			auto worktree_name = get_path(dirname.filename());
			result.cov_dir = cov_path / "worktrees"sv / worktree_name;
			result.commondir = fmt::format(
			    "{}\n", get_path(rel_path(cov_path, result.cov_dir)));
			result.covdir = fmt::format(
			    "{}\n", get_path(rel_path(cov_dir, result.cov_dir)));
			result.gitdir = fmt::format(
			    "{}\n", get_path(rel_path(git_dir, result.cov_dir)));
			result.HEAD = fmt::format("{}{}\n", names::ref_prefix, ref->name());
			return result;
		}
	}  // namespace

	path init_repository(path const& cov_dir,
	                     path const& git_dir,
	                     std::error_code& ec,
	                     init_options const& opts) {
		init_transaction tr{cov_dir};
		path result{};

		if (is_valid_path(cov_dir)) {
			if ((opts.flags & init_options::reinit) == 0) {
				ec = make_error_code(std::errc::file_exists);
				tr.commit();
				return result;
			}
			remove_all(cov_dir, ec);
			if (ec) return result;
		}

		if (opts.flags & init_options::worktree) {
			auto linked =
			    prepare_worktree(cov_dir, git_dir, ec, opts.branch_name);
			if (ec) return result;

			if (is_valid_path(linked.cov_dir)) {
				if ((opts.flags & init_options::reinit) == 0) {
					ec = make_error_code(std::errc::file_exists);
					tr.commit();
					return result;
				}
				remove_all(linked.cov_dir, ec);
				if (ec) return result;
			}

			create_directories(linked.cov_dir, ec);
			if (ec) return result;

			struct {
				std::string_view filename;
				std::string_view contents;
			} const linked_files[] = {{names::commondir_link, linked.commondir},
			                          {names::covdir_link, linked.covdir},
			                          {names::gitdir_link, linked.gitdir},
			                          {names::HEAD, linked.HEAD}};

			for (auto const [filename, contents] : linked_files) {
				ec = touch(linked.cov_dir / filename, contents);
				if (ec) return result;
			}

			ec =
			    touch(cov_dir,
			          fmt::format("{}{}\n", names::covlink_prefix,
			                      get_path(std::filesystem::relative(
			                          linked.cov_dir, cov_dir.parent_path()))));
			if (ec) return result;

			ec.clear();
			tr.commit();
			result = std::move(linked.cov_dir);
			return result;
		}

		for (auto const dirname : {names::objects_pack_dir, names::coverage_dir,
		                           names::heads_dir, names::tags_dir}) {
			create_directories(cov_dir / dirname, ec);
			if (ec) return result;
		}

		if ((ec = init_config(cov_dir, git_dir))) {
			// GCOV_EXCL_START -- untestable without a create_directories
			// already reporting an error
			return result;
		}  // GCOV_EXCL_STOP

		ec = touch(cov_dir / names::HEAD, "ref: refs/heads/main\n"sv);
		if (ec) return result;

			// TODO: init_hilites

#ifdef _WIN32
		if (auto const last = cov_dir.filename();
		    !last.empty() && *last.c_str() == '.') {
			auto const prev = GetFileAttributesW(cov_dir.c_str());
			if (prev != INVALID_FILE_ATTRIBUTES) {
				SetFileAttributesW(cov_dir.c_str(),
				                   prev | FILE_ATTRIBUTE_HIDDEN);
			}
		}
#endif

		ec.clear();
		tr.commit();
		result = std::move(tr.root);
		return result;
	}
}  // namespace cov
