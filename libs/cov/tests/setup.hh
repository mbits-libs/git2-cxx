// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>

namespace cov::testing::setup {
	using std::filesystem::path;
	using namespace ::std::literals;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
	std::string get_path(path const& p);
	path make_path(std::string_view utf8);
}  // namespace cov::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os);
