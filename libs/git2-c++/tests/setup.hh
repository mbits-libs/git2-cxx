// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <git2/repository.hh>
#include <string>

namespace git::testing::setup {
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
	git::repository open_repo(std::error_code& ec);

	namespace hash {
		static constexpr auto commit =
		    "b3f67d63bd1c87427805eba4aa028bfa43587f78"sv;
		static constexpr auto tree =
		    "18983d0294c54064191fc6f20b158ab11747c27d"sv;
		static constexpr auto README =
		    "915365304fffc2cd8fe5b8e2b28cfcf94fdcf9de"sv;
	}  // namespace hash
}  // namespace git::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os);
