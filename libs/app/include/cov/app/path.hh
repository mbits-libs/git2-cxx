// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace cov::app {
	inline std::filesystem::path make_u8path(std::string_view u8) {
		return std::u8string_view{reinterpret_cast<char8_t const*>(u8.data()),
		                          u8.size()};
	}

	inline std::string get_u8path(std::filesystem::path copy) {
		copy.make_preferred();
		auto u8 = copy.u8string();
		return {reinterpret_cast<char const*>(u8.data()), u8.size()};
	}
}  // namespace cov::app
