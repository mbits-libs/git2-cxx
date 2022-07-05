// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "hilite/none.hh"
#include "support.hh"

#include <fmt/format.h>
#include <gtest/gtest.h>

#undef HILITE_TOKENS
#define TESTED_TOKENS(X) \
	X(whitespace)        \
	X(newline)           \
	X(line_comment)      \
	X(block_comment)     \
	X(identifier)        \
	X(keyword)           \
	X(module_name)       \
	X(known_ident_1)     \
	X(known_ident_2)     \
	X(known_ident_3)     \
	X(punctuator)        \
	X(number)            \
	X(character)         \
	X(char_encoding)     \
	X(char_delim)        \
	X(char_udl)          \
	X(string)            \
	X(string_encoding)   \
	X(string_delim)      \
	X(string_udl)        \
	X(escape_sequence)   \
	X(raw_string)        \
	X(meta)              \
	X(meta_identifier)

namespace highlighter::testing {
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class token : unsigned {
#define LIST_TOKENS(x) x = hl::token::x,
		TESTED_TOKENS(LIST_TOKENS)
#undef LIST_TOKENS
		    outside_list,
	};

	struct none_test {
		token value;
		std::string_view expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                none_test const& test) {
			return print_view(out, test.expected);
		}
	};

	class none : public TestWithParam<none_test> {};

	TEST_P(none, names) {
		auto const& [value, expected] = GetParam();
		auto actual = hl::none::token_to_string(static_cast<unsigned>(value));
		ASSERT_EQ(expected, actual);
	}

#define STRINGIFY(x) {token::x, #x##sv},
	static const none_test tests[] = {
	    TESTED_TOKENS(STRINGIFY)  //
	    {token::outside_list, {}},
	};
#undef STRINGIFY

	INSTANTIATE_TEST_SUITE_P(tests, none, ::testing::ValuesIn(tests));
}  // namespace highlighter::testing
