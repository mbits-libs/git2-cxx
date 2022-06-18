// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>

namespace git::testing::setup {
	struct test_initializer {
		test_initializer();
		~test_initializer();
	};
	static test_initializer initializer{};

	std::filesystem::path test_dir();
}  // namespace git::testing::setup
