// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include "cell/ascii.hh"
#include "cell/context.hh"
#include "cell/repeat_operators.hh"
#include "cell/special.hh"
#include "support.hh"

namespace cell::testing {
	using namespace std::literals;
	using null_context =
	    context<std::string_view::iterator, as_parser_t<epsilon>, int>;

	TEST(cell, repeat) {
		auto const parse = [](std::string_view v) {
			static constexpr auto working9to5 = repeat(5, 9)(digit);
			null_context ctx{{}, {}};
			auto it = v.begin();
			return working9to5.parse(it, v.end(), ctx);
		};

		EXPECT_TRUE(parse("123456789"sv));
		EXPECT_TRUE(parse("12345678"sv));
		EXPECT_TRUE(parse("1234567"sv));
		EXPECT_TRUE(parse("12345"sv));
		EXPECT_TRUE(parse("12345abc"sv));
		EXPECT_FALSE(parse("12a456789"sv));
	}
}  // namespace cell::testing
