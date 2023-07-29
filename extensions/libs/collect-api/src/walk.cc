// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "walk.hh"

#include <fmt/format.h>
#include <cov/app/path.hh>
#include <string>

#ifdef WIN32
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
namespace cov::app::platform {
	bool glob_matches(std::filesystem::path const& match,
	                  std::filesystem::path const name) {
		return !!PathMatchSpecW(name.c_str(), match.c_str());
	}
}  // namespace cov::app::platform
#else
#include <fnmatch.h>
int fnmatch(const char* pattern, const char* string, int flags);
namespace cov::app::platform {
	bool glob_matches(std::filesystem::path const& match,
	                  std::filesystem::path const name) {
		return !fnmatch(match.c_str(), name.c_str(), FNM_PATHNAME);
	}
}  // namespace cov::app::platform
#endif

using namespace std::literals;

namespace cov::app::collect {
	std::vector<std::pair<walk_type, std::filesystem::path>> split_path(
	    std::filesystem::path const& path) {
		std::vector<std::pair<walk_type, std::filesystem::path>> tmp{};

		for (auto const& part : path) {
			auto const u8part = get_u8path(part);
			auto const kind =
			    u8part == "**"sv ? walk_type::recursive
			    : u8part.find_first_of("*?"sv) != std::string::npos
			        ? walk_type::directory_glob
			        : walk_type::directory;
			tmp.push_back({kind, part});
		}

		if (!tmp.empty()) {
			switch (tmp.back().first) {
				case walk_type::directory:
					tmp.back().first = walk_type::filename;
					break;
				case walk_type::directory_glob:
					tmp.back().first = walk_type::filename_glob;
					break;
				default:
					break;
			}
		}
		std::vector<std::pair<walk_type, std::filesystem::path>> rewrite{};
		for (auto& [kind, part] : tmp) {
			if (!rewrite.empty() && rewrite.back().first == kind &&
			    (kind == walk_type::directory ||
			     kind == walk_type::recursive)) {
				rewrite.back().second /= part;
				continue;
			}

			rewrite.push_back({kind, part});
		}

		return rewrite;
	}

	static void walk_impl(
	    std::filesystem::path const& root,
	    std::span<std::pair<walk_type, std::filesystem::path> const> stack,
	    std::function<void(std::filesystem::path const&)> const& cb);

	static void notify(
	    std::filesystem::path const& path,
	    std::function<void(std::filesystem::path const&)> const& cb) {
		std::error_code ec{};
		auto exists = std::filesystem::is_regular_file(path, ec);
		if (!ec && exists) cb(path);
	}

	static void walk_notify(
	    std::filesystem::path const& path,
	    std::filesystem::path const& mask,
	    std::function<void(std::filesystem::path const&)> const& cb) {
		using namespace std::filesystem;

		std::error_code ec{};
		directory_iterator entries{path, ec};
		if (ec) return;

		for (auto const& entry : entries) {
			if (!is_regular_file(entry.status())) continue;
			if (!platform::glob_matches(mask, entry.path().filename()))
				continue;
			cb(entry.path());
		}
	}

	static void walk_directory(
	    std::filesystem::path const& path,
	    std::filesystem::path const& mask,
	    std::span<std::pair<walk_type, std::filesystem::path> const> stack,
	    std::function<void(std::filesystem::path const&)> const& cb) {
		using namespace std::filesystem;

		std::error_code ec{};
		directory_iterator entries{path, ec};
		if (ec) return;

		for (auto const& entry : entries) {
			if (!is_directory(entry.status())) continue;
			if (!platform::glob_matches(mask, entry.path().filename()))
				continue;

			walk_impl(entry.path(), stack, cb);
		}
	}

	static void recurse(
	    std::filesystem::path const& root,
	    std::span<std::pair<walk_type, std::filesystem::path> const> stack,
	    std::function<void(std::filesystem::path const&)> const& cb) {
		using namespace std::filesystem;

		// first, `.`:
		walk_impl(root, stack, cb);

		std::error_code ec{};
		recursive_directory_iterator entries{root, ec};
		if (ec) return;

		for (auto const& entry : entries) {
			if (!is_directory(entry.status())) continue;
			walk_impl(entry.path(), stack, cb);
		}
	}

	static void walk_impl(
	    std::filesystem::path const& root,
	    std::span<std::pair<walk_type, std::filesystem::path> const> stack,
	    std::function<void(std::filesystem::path const&)> const& cb) {
		if (stack.empty()) return;
		auto [type, chunk] = stack.front();
		stack = stack.subspan(1);

		switch (type) {
			case walk_type::filename:
				notify(root / chunk, cb);
				return;
			case walk_type::filename_glob:
				walk_notify(root, chunk, cb);
				return;
			case walk_type::directory:
				walk_impl(root / chunk, stack, cb);
				return;
			case walk_type::directory_glob:
				walk_directory(root, chunk, stack, cb);
				return;
			case walk_type::recursive:
				if (stack.empty()) return;
				recurse(root, stack, cb);
				return;
		}
	}

	void walk_split(
	    std::span<std::pair<walk_type, std::filesystem::path> const> stack,
	    std::function<void(std::filesystem::path const&)> const& cb) {
		std::filesystem::path root{"."sv};
		if (!stack.empty() && stack.front().first == walk_type::directory) {
			root = stack.front().second;
			stack = stack.subspan(1);
		}

		walk_impl(root, stack, cb);
	}

	void walk(std::filesystem::path const& path,
	          std::function<void(std::filesystem::path const&)> const& cb) {
		walk_split(split_path(path), cb);
	}
}  // namespace cov::app::collect
