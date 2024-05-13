// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cxx-filt/expression.hh>
#include <cxx-filt/parser.hh>
#include <string_view>

namespace cxx_filt::testing {
	using namespace ::std::literals;

#define TOKEN_TEST_EX(TOKEN, STR, DBG)                    \
	TEST(parser, tokens_str_##TOKEN) {                    \
		auto const expected = STR##sv;                    \
		auto const actual = str_from_token(Token::TOKEN); \
		ASSERT_EQ(expected, actual);                      \
	}                                                     \
	TEST(parser, tokens_dbg_##TOKEN) {                    \
		auto const expected = DBG##sv;                    \
		auto const actual = dbg_from_token(Token::TOKEN); \
		ASSERT_EQ(expected, actual);                      \
	}
#define TOKEN_TEST(TOKEN, STR) TOKEN_TEST_EX(TOKEN, STR, "[" STR "]")

	TOKEN_TEST_EX(NONE, "", "<NONE>");
	TOKEN_TEST_EX(SPACE, " ", "<SP>");
	TOKEN_TEST(COMMA, ",");
	TOKEN_TEST(COLON, ":");
	TOKEN_TEST(HASH, "#");
	TOKEN_TEST(SCOPE, "::");
	TOKEN_TEST(PTR, "*");
	TOKEN_TEST(REF, "&");
	TOKEN_TEST(REF_REF, "&&");
	TOKEN_TEST_EX(CONST, " const", "<CONST>");
	TOKEN_TEST_EX(VOLATILE, " volatile", "<VOLATILE>");
	TOKEN_TEST(OPEN_ANGLE_BRACKET, "<");
	TOKEN_TEST(CLOSE_ANGLE_BRACKET, ">");
	TOKEN_TEST(OPEN_PAREN, "(");
	TOKEN_TEST(CLOSE_PAREN, ")");
	TOKEN_TEST(OPEN_CURLY, "{");
	TOKEN_TEST(CLOSE_CURLY, "}");
	TOKEN_TEST_EX(OPEN_BRACKET, "[", "<[>");
	TOKEN_TEST_EX(CLOSE_BRACKET, "]", "<]>");

	static constexpr auto unknown_token = static_cast<Token>(
	    std::numeric_limits<std::underlying_type_t<Token>>::max());
	TEST(parser, tokens_str_unknown) {
		auto const expected = ""sv;
		auto const actual = str_from_token(unknown_token);
		ASSERT_EQ(expected, actual);
	}
	TEST(parser, tokens_dbg_unknown) {
		auto const expected = "[???]"sv;
		auto const actual = dbg_from_token(unknown_token);
		ASSERT_EQ(expected, actual);
	}
}  // namespace cxx_filt::testing
