// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>

namespace git::testing::setup {
	using std::filesystem::path;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
	std::string get_path(path const& p);
	path make_path(std::string_view utf8);
}  // namespace git::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os);
