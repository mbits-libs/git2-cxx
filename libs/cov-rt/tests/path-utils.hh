// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/init.hh>
#include <cov/io/file.hh>
#include <git2/repository.hh>
#include <string>
#include <vector>
#include "setup.hh"

namespace cov::app::testing {
	using namespace ::std::literals;
	using namespace ::std::filesystem;

	enum class path_kind {
		remove_all,
		create_directories,
		touch,
		init_repo,
		init_git_workspace,
		init_bare_git
	};

	struct path_info {
		std::string_view name;
		path_kind kind{path_kind::create_directories};
		std::string_view second{};

		void op(std::error_code& ec) const {
			switch (kind) {
				case path_kind::remove_all:
					remove_all(setup::test_dir() / name, ec);
					return;
				case path_kind::create_directories:
					create_directories(setup::test_dir() / name, ec);
					return;
				case path_kind::init_repo:
					init_repository(setup::test_dir() / name,
					                setup::test_dir() / second, ec);
					return;
				case path_kind::init_git_workspace:
					git::repository::init(setup::test_dir() / name, false, ec);
					return;
				case path_kind::init_bare_git:
					git::repository::init(setup::test_dir() / name, true, ec);
					return;
				case path_kind::touch: {
					auto path = setup::test_dir() / name;
					create_directories(path.parent_path(), ec);
					if (ec) return;
					auto touch = io::fopen(path, "w");
					if (touch)
						touch.store(second.data(), second.length());
					else
						ec = std::make_error_code(std::errc::permission_denied);
					return;
				}
			}
		}

		static void op(std::vector<path_info> const& items,
		               std::error_code& ec) {
			for (auto const& item : items) {
				item.op(ec);
				if (ec) return;
			}
		}
	};

	template <std::same_as<path_info>... PathInfo>
	std::vector<path_info> make_setup(PathInfo&&... items) {
		return {items...};
	}

	consteval path_info remove_all(std::string_view name) {
		return {name, path_kind::remove_all};
	}

	consteval path_info create_directories(std::string_view name) {
		return {name, path_kind::create_directories};
	}

	consteval path_info touch(std::string_view name,
	                          std::string_view text = {}) {
		return {name, path_kind::touch, text};
	}

	consteval path_info init_repo(std::string_view name,
	                              std::string_view git = {}) {
		return {name, path_kind::init_repo, git};
	}

	consteval path_info init_bare_git(std::string_view name) {
		return {name, path_kind::init_bare_git};
	}

	consteval path_info init_git_workspace(std::string_view name) {
		return {name, path_kind::init_git_workspace};
	}

	struct entry_t {
		std::string filename;
		file_type type;

		bool operator==(entry_t const& other) const noexcept {
			return filename == other.filename && type == other.type;
		}

		bool operator<(entry_t const& other) const noexcept {
			if (filename == other.filename) return type < other.type;
			return filename < other.filename;
		}

		friend std::ostream& operator<<(std::ostream& out,
		                                entry_t const& entry) {
			return out << "{\"" << entry.filename << "\", "
			           << static_cast<int>(entry.type) << '}';
		}
	};
}  // namespace cov::app::testing
