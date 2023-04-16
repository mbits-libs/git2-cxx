// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace testing::path {
	std::string get_path(std::filesystem::path const& p);
	std::filesystem::path make_u8path(std::string_view utf8);
}  // namespace testing::path

void PrintTo(std::filesystem::path const& path, ::std::ostream* os);
