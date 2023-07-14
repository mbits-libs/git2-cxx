// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/git2/oid.hh>
#include <filesystem>
#include <string>
#include "path.hh"

namespace cov::testing::setup {
	using namespace ::std::literals;
	using ::testing::path::get_path;
	using ::testing::path::make_u8path;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
	std::string get_oid(git::oid_view id);
}  // namespace cov::testing::setup
