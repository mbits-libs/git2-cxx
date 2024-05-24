// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <git2/oid.h>
#include <cov/git2/repository.hh>
#include <filesystem>
#include <string>
#include "path.hh"

namespace cov::setup {
	using ::testing::path::get_path;
	using ::testing::path::make_u8path;
	using namespace ::std::literals;

	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
	git::repository open_verify_repo();
}  // namespace cov::setup
