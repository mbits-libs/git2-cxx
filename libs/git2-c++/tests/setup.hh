// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <git2/repository.hh>
#include <string>
#include "path.hh"

namespace git::testing::setup {
	using ::testing::path::get_path;
	using ::testing::path::make_u8path;
	using namespace ::std::literals;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
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
