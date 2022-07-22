// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include "cell/parser.hh"
#include "support.hh"

namespace lighter::testing {
	TEST(additional, compare_tokens) {
		using hl::token_t;

		static constexpr auto nl100 = token_t{0, 100, hl::newline};
		static constexpr auto nl90 = token_t{0, 90, hl::newline};
		static constexpr auto nl10_100 = token_t{10, 100, hl::newline};
		static constexpr auto comment100 = token_t{0, 100, hl::line_comment};

		ASSERT_EQ(nl100, nl100);
		ASSERT_EQ(nl90, nl90);
		ASSERT_EQ(nl10_100, nl10_100);
		ASSERT_EQ(comment100, comment100);
		ASSERT_NE(nl100, nl90);
		ASSERT_NE(nl90, nl100);
		ASSERT_NE(nl100, nl10_100);
		ASSERT_NE(nl10_100, nl100);
		ASSERT_NE(nl100, comment100);
		ASSERT_NE(comment100, nl100);

		ASSERT_FALSE(nl100 < comment100);
		ASSERT_FALSE(comment100 < nl100);
		ASSERT_GT(nl90, nl100);
		ASSERT_LT(nl100, nl90);
		ASSERT_GT(nl10_100, nl100);
		ASSERT_LT(nl100, nl10_100);

		ASSERT_TRUE(nl100.cmp(comment100) < 0);  // NOLINT
		ASSERT_TRUE(comment100.cmp(nl100) > 0);  // NOLINT
		ASSERT_TRUE(nl100.cmp(nl90) < 0);        // NOLINT
		ASSERT_TRUE(nl90.cmp(nl100) > 0);        // NOLINT
		ASSERT_TRUE(nl100.cmp(nl10_100) < 0);    // NOLINT
		ASSERT_TRUE(nl10_100.cmp(nl100) > 0);    // NOLINT
	}

	TEST(additional, action_state) {
		ASSERT_TRUE(cell::action_state::enabled());
		cell::action_state off{};
		ASSERT_FALSE(cell::action_state::enabled());
	}
}  // namespace lighter::testing
