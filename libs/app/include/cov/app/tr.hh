// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <string_view>
#include <fmt/format.h>

namespace cov::app {
	static inline constexpr std::string_view _(std::string_view in) { return in; }
	template <size_t Len>
	static inline constexpr std::string_view _(char const (&in)[Len]) {
		return {in, Len - 1};
	}
}  // namespace cov::app