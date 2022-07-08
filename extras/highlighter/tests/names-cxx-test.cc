// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "support.hh"

#include <fmt/format.h>
#include <gtest/gtest.h>

#ifdef __has_include
#if __has_include("hilite/cxx.hh")
#define HAS_CXX 1
#endif
#endif

#ifdef HAS_CXX

#include "hilite/cxx.hh"

#undef HILITE_TOKENS
#undef CXX_HILITE_TOKENS
#undef CXX_TOKENS

#define TESTED_TOKENS(X)        \
	X(whitespace)               \
	X(newline)                  \
	X(line_comment)             \
	X(block_comment)            \
	X(identifier)               \
	X(keyword)                  \
	X(module_name)              \
	X(known_ident_1)            \
	X(known_ident_2)            \
	X(known_ident_3)            \
	X(punctuator)               \
	X(number)                   \
	X(character)                \
	X(char_encoding)            \
	X(char_delim)               \
	X(char_udl)                 \
	X(string)                   \
	X(string_encoding)          \
	X(string_delim)             \
	X(string_udl)               \
	X(escape_sequence)          \
	X(raw_string)               \
	X(meta)                     \
	X(meta_identifier)          \
	X(deleted_newline)          \
	X(local_header_name)        \
	X(system_header_name)       \
	X(universal_character_name) \
	X(macro_name)               \
	X(macro_arg_list)           \
	X(macro_arg)                \
	X(macro_va_args)            \
	X(macro_replacement)

namespace highlighter::testing {
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	enum class token : unsigned {
#define LIST_TOKENS(x) x = hl::cxx::x,
		TESTED_TOKENS(LIST_TOKENS)
#undef LIST_TOKENS
		    outside_list,
	};

	struct cxx_test {
		token value;
		std::string_view expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                cxx_test const& test) {
			return print_view(out, test.expected);
		}
	};

	class cxx : public TestWithParam<cxx_test> {};

	TEST_P(cxx, names) {
		auto const& [value, expected] = GetParam();
		auto actual = hl::cxx::token_to_string(static_cast<unsigned>(value));
		ASSERT_EQ(expected, actual);
	}

#define STRINGIFY(x) {token::x, #x##sv},
	static const cxx_test tests[] = {
	    TESTED_TOKENS(STRINGIFY)  //
	    {token::outside_list, {}},
	};
#undef STRINGIFY

	INSTANTIATE_TEST_SUITE_P(tests, cxx, ::testing::ValuesIn(tests));
}  // namespace highlighter::testing

#endif  // defined(HAS_CXX)
