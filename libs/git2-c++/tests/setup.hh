// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <git2-c++/repository.hh>

namespace git::testing::setup {
	using std::filesystem::path;
	using ::std::literals::operator""sv;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
	std::string get_path(path const& p);
	path make_path(std::string_view utf8);
	git::repository open_repo();

	namespace hash {
		static constexpr auto commit =
		    "ed631389fc343f7788bf414c2b3e77749a15deb6"sv;
		static constexpr auto tree =
		    "71bb790a4e8eb02b82f948dfb13d9369b768d047"sv;
		static constexpr auto README =
		    "d85c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv;
	}  // namespace hash
}  // namespace git::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os);
