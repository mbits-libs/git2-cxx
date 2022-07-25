// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/init.hh>
#include <cov/io/file.hh>
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <git2/config.hh>
#include <git2/error.hh>
#include <git2/repository.hh>
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

		std::error_code init_config(path const& base_dir, path const& git_dir) {
			std::error_code ec{};
			auto cfg = git::config::create(ec);
			if (!cfg) return ec;

			if ((ec = cfg.add_file_ondisk(
			         get_path(base_dir / names::config).c_str()))) {
				// GCOV_EXCL_START -- untestable, git_add_file_ondisk returns
				// error, if the stat failed, not due to ENOENT/ENOTDIR;
				// however, this would require messing with covdata directory,
				// which would be detected by create_directories below
				// And put something here as well...
				return ec;
				// GCOV_EXCL_STOP
			}

			return cfg.set_path(names::core_gitdir,
			                    rel_path(git_dir, base_dir));
		}
	}  // namespace

	path init_repository(path const& base_dir,
	                     path const& git_dir,
	                     std::error_code& ec,
	                     init_options const& opts) {
		init_transaction tr{base_dir};
		path result{};

		if (is_valid_path(base_dir)) {
			if ((opts.flags & init_options::reinit) == 0) {
				ec = make_error_code(std::errc::file_exists);
				tr.commit();
				return result;
			}
			remove_all(base_dir, ec);
			if (ec) return result;
		}

		for (auto const dirname : {names::objects_pack_dir, names::coverage_dir,
		                           names::heads_dir, names::tags_dir}) {
			create_directories(base_dir / dirname, ec);
			if (ec) return result;
		}

		if ((ec = init_config(base_dir, git_dir))) {
			// GCOV_EXCL_START -- untestable without a create_directories
			// already reporting an error
			return result;
			// GCOV_EXCL_STOP
		}

		if (auto const touch = io::fopen(base_dir / names::HEAD, "wb");
		    !touch) {
			auto const error = errno;
			ec = std::make_error_code(error ? static_cast<std::errc>(error)
			                                : std::errc::permission_denied);
			return result;
		} else {  // GCOV_EXCL_LINE[WIN32]
			static constexpr auto ref = "ref: refs/heads/main\n"sv;
			touch.store(ref.data(), ref.size());
		}

		// TODO: init_hilites

#ifdef _WIN32
		if (auto const last = base_dir.filename();
		    !last.empty() && *last.c_str() == '.') {
			auto const prev = GetFileAttributesW(base_dir.c_str());
			if (prev != INVALID_FILE_ATTRIBUTES) {
				SetFileAttributesW(base_dir.c_str(),
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
