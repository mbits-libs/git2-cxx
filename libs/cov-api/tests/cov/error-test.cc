// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/error.hh>

namespace cov::testing {
	using namespace ::std::literals;

#define ON_ERRC(X)       \
	X(error)             \
	X(current_branch)    \
	X(wrong_object_type) \
	X(uninitialized_worktree) X(not_a_worktree) X(not_a_branch)

	TEST(error, same_error) {
#define X_ASSERT_EQ(ERR) ASSERT_EQ(errc::ERR, cov::make_error_code(errc::ERR));
		ON_ERRC(X_ASSERT_EQ)
#undef X_ASSERT_EQ
	}

	TEST(error, has_message) {
#define X_HAS_MESSAGE(ERR) \
	ASSERT_FALSE(cov::make_error_code(errc::ERR).message().empty());
		ON_ERRC(X_HAS_MESSAGE)
#undef X_HAS_MESSAGE
	}

	TEST(error, unknown_has_message) {
		ASSERT_EQ("cov::errc{100}",
		          cov::make_error_code(static_cast<errc>(100)).message());
	}

	TEST(error, category_has_name) {
		ASSERT_STREQ(cov::category().name(),
		             cov::make_error_code(errc::error).category().name());
		// same pointer:
		ASSERT_EQ(cov::category().name(),
		          cov::make_error_code(errc::error).category().name());
	}
}  // namespace cov::testing
