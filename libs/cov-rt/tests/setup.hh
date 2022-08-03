// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <git2/oid.h>
#include <filesystem>
#include <string>
#include "path.hh"

namespace cov::testing::setup {
	using ::testing::path::get_path;
	using ::testing::path::make_u8path;
	using namespace ::std::literals;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
	std::string get_oid(git_oid const& id);
}  // namespace cov::testing::setup
