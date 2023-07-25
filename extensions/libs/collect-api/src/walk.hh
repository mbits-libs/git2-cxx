// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace cov::app::collect {
	enum class walk_type {
		directory,
		directory_glob,
		filename,
		filename_glob,
		recursive,
	};

	std::vector<std::pair<walk_type, std::filesystem::path>> split_path(
	    std::filesystem::path const& path);

	void walk_split(
	    std::span<std::pair<walk_type, std::filesystem::path> const>,
	    std::function<void(std::filesystem::path const&)> const&);

	void walk(std::filesystem::path const& path,
	          std::function<void(std::filesystem::path const&)> const&);
}  // namespace cov::app::collect
