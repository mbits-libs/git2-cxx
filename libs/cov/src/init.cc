// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/init.hh>
#include <cov/io/file.hh>
#include <cov/object.hh>
#include <cov/types.hh>
#include <fstream>
#include <git2-c++/config.hh>
#include <git2-c++/error.hh>
#include <git2-c++/repository.hh>
#include "path-utils.hh"

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

		int init_config(path const& base_dir, path const& git_dir) {
			auto cfg = git::config::create();
			if (!cfg) return -1;

			if (auto err = cfg.add_file_ondisk(
			        get_path(base_dir / names::config).c_str()))
				return err;

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

		if (auto const err = init_config(base_dir, git_dir)) {
			ec = git::make_error_code(err);
			return result;
		}

		if (auto const touch = io::fopen(base_dir / names::HEAD, "wb");
		    !touch) {
			auto const error = errno;
			ec = std::make_error_code(error ? static_cast<std::errc>(error)
			                                : std::errc::permission_denied);
			return result;
		} else {
			static constexpr auto ref = "ref: refs/heads/main\n"sv;
			touch.store(ref.data(), ref.size());
		}

		// TODO: init_hilites

		ec.clear();
		tr.commit();
		result = std::move(tr.root);
		return result;
	}
}  // namespace cov
