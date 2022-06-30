// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <git2/bytes.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;

	static constexpr auto digits = "0123456789"sv;

	std::string_view to_view(bytes b) {
		return {reinterpret_cast<char const*>(b.data()), b.size()};
	}

	TEST(bytes, offset_only) {
		ASSERT_EQ(digits, to_view(bytes{digits}.subview()));
		ASSERT_EQ(digits, to_view(bytes{digits}.subview(0)));
		ASSERT_EQ(digits.substr(3), to_view(bytes{digits}.subview(3)));
	}

	TEST(bytes, length_only) {
		ASSERT_EQ(digits.substr(0, 3), to_view(bytes{digits}.subview(0, 3)));
		ASSERT_EQ(digits.substr(0, 7), to_view(bytes{digits}.subview(0, 7)));
		ASSERT_EQ(digits, to_view(bytes{digits}.subview(0, 10)));
		ASSERT_EQ(digits, to_view(bytes{digits}.subview(0, 15)));
	}

	TEST(bytes, subview) {
		ASSERT_EQ(digits.substr(2, 3), to_view(bytes{digits}.subview(2, 3)));
		ASSERT_EQ(digits.substr(2, 7), to_view(bytes{digits}.subview(2, 7)));
		ASSERT_EQ(digits.substr(5, 5), to_view(bytes{digits}.subview(5, 7)));
	}
}  // namespace git::testing
