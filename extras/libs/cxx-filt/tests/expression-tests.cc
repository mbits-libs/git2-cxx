// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cxx-filt/expression.hh>
#include <cxx-filt/parser.hh>
#include <string_view>

namespace cxx_filt::testing {
	using namespace ::std::literals;

	struct expression_test {
		std::string_view input;
		struct {
			std::string_view str;
			std::string_view repr;
			std::string_view simplified;
		} expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                expression_test const& test) {
			return out << test.input;
		}
	};

	class expression : public ::testing::TestWithParam<expression_test> {
	protected:
		static Replacements replacements;

		static void SetUpTestSuite() {
			static constexpr std::pair<std::string_view, std::string_view>
			    raw_replacements[] = {
			        {"std::__cxx11::basic_string<$1, $2, $3>"sv,
			         "std::basic_string<$1, $2, $3>"sv},
			        {"std::basic_string<$1, $2, std::allocator<$1>>"sv,
			         "std::basic_string<$1, $2>"sv},
			        {"std::basic_string<$1, std::char_traits<$1>>"sv,
			         "std::basic_string<$1>"sv},
			        {"std::basic_string<$1, $2, std::pmr::polymorphic_allocator<$1>>"sv,
			         "std::pmr::basic_string<$1, $2>"sv},
			        {"std::pmr::basic_string<$1, std::char_traits<$1>>"sv,
			         "std::pmr::basic_string<$1>"sv},
			        {"std::basic_string<char>"sv, "std::string"sv},
			        {"std::basic_string<wchar_t>"sv, "std::wstring"sv},
			        {"std::basic_string<char8_t>"sv, "std::u8string"sv},
			        {"std::basic_string<char16_t>"sv, "std::u8string"sv},
			        {"std::basic_string<char32_t>"sv, "std::u32string"sv},
			        {"std::pmr::basic_string<char>"sv, "std::pmr::string"sv},
			        {"std::pmr::basic_string<wchar_t>"sv,
			         "std::pmr::wstring"sv},
			        {"std::pmr::basic_string<char8_t>"sv,
			         "std::pmr::u8string"sv},
			        {"std::pmr::basic_string<char16_t>"sv,
			         "std::pmr::u8string"sv},
			        {"std::pmr::basic_string<char32_t>"sv,
			         "std::pmr::u32string"sv},

			        {"std::basic_string_view<$1, std::char_traits<$1>>"sv,
			         "std::basic_string_view<$1>"sv},
			        {"std::basic_string_view<char>"sv, "std::string_view"sv},
			        {"std::basic_string_view<wchar_t>"sv,
			         "std::wstring_view"sv},
			        {"std::basic_string_view<char8_t>"sv,
			         "std::u8string_view"sv},
			        {"std::basic_string_view<char16_t>"sv,
			         "std::u16string_view"sv},
			        {"std::basic_string_view<char32_t>"sv,
			         "std::u32string_view"sv},

			        {"std::ratio<$1, 1l>"sv, "std::ratio<$1>"sv},
			        {"std::ratio<1l, 1000000000000000000000000000000l>"sv,
			         "std::quecto"sv},
			        {"std::ratio<1l, 1000000000000000000000000000l>"sv,
			         "std::ronto"sv},
			        {"std::ratio<1l, 1000000000000000000000000l>"sv,
			         "std::yocto"sv},
			        {"std::ratio<1l, 1000000000000000000000l>"sv,
			         "std::zepto"sv},
			        {"std::ratio<1l, 1000000000000000000l>"sv, "std::atto"sv},
			        {"std::ratio<1l, 1000000000000000l>"sv, "std::femto"sv},
			        {"std::ratio<1l, 1000000000000l>"sv, "std::pico"sv},
			        {"std::ratio<1l, 1000000000l>"sv, "std::nano"sv},
			        {"std::ratio<1l, 1000000l>"sv, "std::micro"sv},
			        {"std::ratio<1l, 1000l>"sv, "std::milli"sv},
			        {"std::ratio<1l, 100l>"sv, "std::centi"sv},
			        {"std::ratio<1l, 10l>"sv, "std::deci"sv},
			        {"std::ratio<1l>"sv, "std::ratio<>"sv},
			        {"std::ratio<10l>"sv, "std::deca"sv},
			        {"std::ratio<100l>"sv, "std::hecto"sv},
			        {"std::ratio<1000l>"sv, "std::kilo"sv},
			        {"std::ratio<1000000l>"sv, "std::mega"sv},
			        {"std::ratio<1000000000l>"sv, "std::giga"sv},
			        {"std::ratio<1000000000000l>"sv, "std::tera"sv},
			        {"std::ratio<1000000000000000l>"sv, "std::peta"sv},
			        {"std::ratio<1000000000000000000l>"sv, "std::exa"sv},
			        {"std::ratio<1000000000000000000000l>"sv, "std::zetta"sv},
			        {"std::ratio<1000000000000000000000000l>"sv,
			         "std::yotta"sv},
			        {"std::ratio<1000000000000000000000000000l>"sv,
			         "std::ronna"sv},
			        {"std::ratio<1000000000000000000000000000000l>"sv,
			         "std::quetta"sv},

			        {"std::chrono::duration<$1, std::ratio<>>"sv,
			         "std::chrono::duration<$1>"sv},
			        {"std::chrono::duration<long, std::nano>"sv,
			         "std::chrono::nanoseconds"sv},
			        {"std::chrono::duration<long, std::micro>"sv,
			         "std::chrono::microseconds"sv},
			        {"std::chrono::duration<long, std::milli>"sv,
			         "std::chrono::milliseconds"sv},
			        {"std::chrono::duration<long>"sv, "std::chrono::seconds"sv},
			        {"std::chrono::duration<long, std::ratio<60l>>"sv,
			         "std::chrono::minutes"sv},
			        {"std::chrono::duration<long, std::ratio<3600l>>"sv,
			         "std::chrono::hours"sv},
			        {"std::chrono::duration<long, std::ratio<86400l>>"sv,
			         "std::chrono::days"sv},
			        {"std::chrono::duration<long, std::ratio<604800l>>"sv,
			         "std::chrono::weeks"sv},
			        {"std::chrono::duration<long, std::ratio<2629746l>>"sv,
			         "std::chrono::months"sv},
			        {"std::chrono::duration<long, std::ratio<31556952l>>"sv,
			         "std::chrono::years"sv},

			        {"std::vector<$1, std::allocator<$1>>"sv,
			         "std::vector<$1>"sv},
			        {"map<$1, $2, alloc<$1 const, $2>>"sv, "map<$1, $2>"sv},
			    };

			replacements.clear();
			replacements.reserve(std::size(raw_replacements));

			for (auto const& [from, to] : raw_replacements) {
				auto stmt_from = Parser::statement_from(from);
				auto stmt_to = Parser::statement_from(to);
				if (stmt_from.items.size() != 1) {
					fmt::print(stderr,
					           "Expected to get exactly one expression from "
					           "'{}', got {}\n",
					           from, stmt_from.items.size());
					return;
				}
				if (stmt_to.items.size() != 1) {
					fmt::print(stderr,
					           "Expected to get exactly one expression from "
					           "'{}', got {}\n",
					           to, stmt_to.items.size());
					return;
				}
				replacements.push_back({std::move(stmt_from.items[0]),
				                        std::move(stmt_to.items[0])});
			}
		}
	};

	Replacements expression::replacements{};

	TEST_P(expression, str) {
		auto const& [input, expected] = GetParam();

		auto const actual = Parser::statement_from(input).str();

		ASSERT_EQ(expected.str, actual);
	}

	TEST_P(expression, repr) {
		auto const& [input, expected] = GetParam();

		auto const actual = Parser::statement_from(input).repr();

		ASSERT_EQ(expected.repr, actual);
	}

	TEST_P(expression, simplified) {
		auto const& [input, expected] = GetParam();

		auto const actual =
		    Parser::statement_from(input).simplified(replacements).str();

		ASSERT_EQ(expected.simplified, actual);
	}

	TEST(expression, args_unmatched_start_stop) {
		auto const paren = ArgumentList{.start = Token::OPEN_PAREN,
		                                .stop = Token::CLOSE_PAREN};
		auto const curly = ArgumentList{.start = Token::OPEN_CURLY,
		                                .stop = Token::CLOSE_CURLY};
		auto const almost_paren = ArgumentList{
		    .start = Token::OPEN_PAREN, .stop = Token::CLOSE_ANGLE_BRACKET};
		Refs refs{};
		ASSERT_FALSE(paren.matches(curly, refs));
		ASSERT_FALSE(paren.matches(almost_paren, refs));
	}

	TEST(expression, expr_unmatched_variants) {
		auto const base = Expression{
		    .items = {"name"s, ArgumentList{}, Reference{.id = "nowhere"s}}};
		auto const first = Expression{
		    .items = {"name"s, "other-names"s, Reference{.id = "nowhere"s}}};
		auto const second = Expression{
		    .items = {"name"s, ArgumentList{}, Reference{.id = "nowhere"s}}};
		Refs refs{};
		ASSERT_FALSE(base.matches(first, refs, cvr_eq::unimportant));
		ASSERT_FALSE(base.matches(second, refs, cvr_eq::unimportant));
	}

	TEST(expression, stmt_unmatched_items) {
		Refs refs{};
		ASSERT_FALSE(Parser::statement_from("a b()").matches(
		    Parser::statement_from("b()"), refs));
	}

	TEST(expression, bad_reference_replacement) {
		Refs refs{};
		auto const actual =
		    Parser::statement_from("$1 const").replace_with(refs);
		ASSERT_EQ("? const"sv, actual.str());
	}

	static constexpr expression_test string[] = {
	    {.input{}, .expected{.str{}, .repr{}, .simplified{}}},

	    {.input =
	         "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>"sv,
	     .expected =
	         {.str =
	              "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>"sv,
	          .repr =
	              "Expr[]{\"std::__cxx11::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}, Expr[]{\"std::allocator\", <Expr[]{\"char\"}>}>}"sv,
	          .simplified = "std::string"sv}},
	    {.input =
	         "std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>"sv,
	     .expected =
	         {.str =
	              "std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>"sv,
	          .repr =
	              "Expr[]{\"std::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}, Expr[]{\"std::pmr::polymorphic_allocator\", <Expr[]{\"char\"}>}>}"sv,
	          .simplified = "std::pmr::string"sv}},
	    {.input =
	         "std::basic_string_view<char8_t, std::char_traits<char8_t>>"sv,
	     .expected =
	         {.str =
	              "std::basic_string_view<char8_t, std::char_traits<char8_t>>"sv,
	          .repr =
	              "Expr[]{\"std::basic_string_view\", <Expr[]{\"char8_t\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char8_t\"}>}>}"sv,
	          .simplified = "std::u8string_view"sv}},
	};

	INSTANTIATE_TEST_SUITE_P(string, expression, ::testing::ValuesIn(string));

	static constexpr expression_test chrono[] = {
	    {.input = "std :: ratio<60l, 1l>"sv,
	     .expected =
	         {.str = "std::ratio<60l, 1l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"60l\"}, Expr[]{\"1l\"}>}"sv,
	          .simplified = "std::ratio<60l>"sv}},
	    {.input = "std ::   ratio<1l, 1000000000000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000000000000000000000l\"}>}"sv,
	          .simplified = "std::quecto"sv}},
	    {.input = "std   ::ratio<1l, 1000000000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000000000000000000l\"}>}"sv,
	          .simplified = "std::ronto"sv}},
	    {.input = "std::   ratio<1l, 1000000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000000000000000l\"}>}"sv,
	          .simplified = "std::yocto"sv}},
	    {.input = " std :: ratio< 1l , 1000000000000000000000l > "sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000000000000l\"}>}"sv,
	          .simplified = "std::zepto"sv}},
	    {.input = "std::ratio<1l,1000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000000000l\"}>}"sv,
	          .simplified = "std::atto"sv}},
	    {.input = "std::ratio<1l, 1000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000000l\"}>}"sv,
	          .simplified = "std::femto"sv}},
	    {.input = "std::ratio<1l, 1000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000000l\"}>}"sv,
	          .simplified = "std::pico"sv}},
	    {.input = "std::ratio<1l, 1000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000l\"}>}"sv,
	          .simplified = "std::nano"sv}},
	    {.input = "std::ratio<1l, 1000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000l\"}>}"sv,
	          .simplified = "std::micro"sv}},
	    {.input = "std::ratio<1l, 1000l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 1000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000l\"}>}"sv,
	          .simplified = "std::milli"sv}},
	    {.input = "std::ratio<1l, 100l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 100l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"100l\"}>}"sv,
	          .simplified = "std::centi"sv}},
	    {.input = "std::ratio<1l, 10l>"sv,
	     .expected =
	         {.str = "std::ratio<1l, 10l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"10l\"}>}"sv,
	          .simplified = "std::deci"sv}},
	    {.input = "std::ratio<1l>"sv,
	     .expected = {.str = "std::ratio<1l>"sv,
	                  .repr = "Expr[]{\"std::ratio\", <Expr[]{\"1l\"}>}"sv,
	                  .simplified = "std::ratio<>"sv}},
	    {.input = "std::ratio<10l>"sv,
	     .expected = {.str = "std::ratio<10l>"sv,
	                  .repr = "Expr[]{\"std::ratio\", <Expr[]{\"10l\"}>}"sv,
	                  .simplified = "std::deca"sv}},
	    {.input = "std::ratio<100l>"sv,
	     .expected = {.str = "std::ratio<100l>"sv,
	                  .repr = "Expr[]{\"std::ratio\", <Expr[]{\"100l\"}>}"sv,
	                  .simplified = "std::hecto"sv}},
	    {.input = "std::ratio<1000l>"sv,
	     .expected = {.str = "std::ratio<1000l>"sv,
	                  .repr = "Expr[]{\"std::ratio\", <Expr[]{\"1000l\"}>}"sv,
	                  .simplified = "std::kilo"sv}},
	    {.input = "std::ratio<1000000l>"sv,
	     .expected = {.str = "std::ratio<1000000l>"sv,
	                  .repr =
	                      "Expr[]{\"std::ratio\", <Expr[]{\"1000000l\"}>}"sv,
	                  .simplified = "std::mega"sv}},
	    {.input = "std::ratio<1000000000l>"sv,
	     .expected = {.str = "std::ratio<1000000000l>"sv,
	                  .repr =
	                      "Expr[]{\"std::ratio\", <Expr[]{\"1000000000l\"}>}"sv,
	                  .simplified = "std::giga"sv}},
	    {.input = "std::ratio<1000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000l>"sv,
	          .repr = "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000l\"}>}"sv,
	          .simplified = "std::tera"sv}},
	    {.input = "std::ratio<1000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000l\"}>}"sv,
	          .simplified = "std::peta"sv}},
	    {.input = "std::ratio<1000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000000l\"}>}"sv,
	          .simplified = "std::exa"sv}},
	    {.input = "std::ratio<1000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000000000l\"}>}"sv,
	          .simplified = "std::zetta"sv}},
	    {.input = "std::ratio<1000000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000000000000l\"}>}"sv,
	          .simplified = "std::yotta"sv}},
	    {.input = "std::ratio<1000000000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000000000000000l\"}>}"sv,
	          .simplified = "std::ronna"sv}},
	    {.input = "std::ratio<1000000000000000000000000000000l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000000000000000000l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000000000000000000l\"}>}"sv,
	          .simplified = "std::quetta"sv}},
	    {.input = "std::ratio<1000000000000000000000000000000l, 1l>"sv,
	     .expected =
	         {.str = "std::ratio<1000000000000000000000000000000l, 1l>"sv,
	          .repr =
	              "Expr[]{\"std::ratio\", <Expr[]{\"1000000000000000000000000000000l\"}, Expr[]{\"1l\"}>}"sv,
	          .simplified = "std::quetta"sv}},

	    {.input = "std::chrono::duration<long, std::ratio<>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <>}>}"sv,
	          .simplified = "std::chrono::seconds"sv}},
	    {.input = "std::chrono::duration<long, std::nano>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::nano>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::nano\"}>}"sv,
	          .simplified = "std::chrono::nanoseconds"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<1l, 1000000000l>>"sv,
	     .expected =
	         {.str =
	              "std::chrono::duration<long, std::ratio<1l, 1000000000l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"1l\"}, Expr[]{\"1000000000l\"}>}>}"sv,
	          .simplified = "std::chrono::nanoseconds"sv}},
	    {.input = "std::chrono::duration<long, std::micro>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::micro>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::micro\"}>}"sv,
	          .simplified = "std::chrono::microseconds"sv}},
	    {.input = "std::chrono::duration<long, std::milli>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::milli>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::milli\"}>}"sv,
	          .simplified = "std::chrono::milliseconds"sv}},
	    {.input = "std::chrono::duration<long>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long>"sv,
	          .repr = "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}>}"sv,
	          .simplified = "std::chrono::seconds"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<60l>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<60l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"60l\"}>}>}"sv,
	          .simplified = "std::chrono::minutes"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<3600l>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<3600l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"3600l\"}>}>}"sv,
	          .simplified = "std::chrono::hours"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<86400l>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<86400l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"86400l\"}>}>}"sv,
	          .simplified = "std::chrono::days"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<604800l>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<604800l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"604800l\"}>}>}"sv,
	          .simplified = "std::chrono::weeks"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<2629746l>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<2629746l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"2629746l\"}>}>}"sv,
	          .simplified = "std::chrono::months"sv}},
	    {.input = "std::chrono::duration<long, std::ratio<31556952l>>"sv,
	     .expected =
	         {.str = "std::chrono::duration<long, std::ratio<31556952l>>"sv,
	          .repr =
	              "Expr[]{\"std::chrono::duration\", <Expr[]{\"long\"}, Expr[]{\"std::ratio\", <Expr[]{\"31556952l\"}>}>}"sv,
	          .simplified = "std::chrono::years"sv}},

	};
	INSTANTIATE_TEST_SUITE_P(chrono, expression, ::testing::ValuesIn(chrono));

	static constexpr expression_test tests[] = {
	    {.input = "map<A, B, alloc<A const, B>>"sv,
	     .expected =
	         {.str = "map<A, B, alloc<A const, B>>"sv,
	          .repr =
	              "Expr[]{\"map\", <Expr[]{\"A\"}, Expr[]{\"B\"}, Expr[]{\"alloc\", <Expr[c]{\"A\"}, Expr[]{\"B\"}>}>}"sv,
	          .simplified = "map<A, B>"sv}},
	    {.input = "map<A const*, B, alloc<A const* const, B>>"sv,
	     .expected =
	         {.str = "map<A const*, B, alloc<A const* const, B>>"sv,
	          .repr =
	              "Expr[]{\"map\", <Expr[c*]{\"A\"}, Expr[]{\"B\"}, Expr[]{\"alloc\", <Expr[c*c]{\"A\"}, Expr[]{\"B\"}>}>}"sv,
	          .simplified = "map<A const*, B>"sv}},
	    {.input = "map<A volatile, B, alloc<A volatile const, B>>"sv,
	     .expected =
	         {.str = "map<A volatile, B, alloc<A const volatile, B>>"sv,
	          .repr =
	              "Expr[]{\"map\", <Expr[v]{\"A\"}, Expr[]{\"B\"}, Expr[]{\"alloc\", <Expr[cv]{\"A\"}, Expr[]{\"B\"}>}>}"sv,
	          /* THIS IS A BUG. EXPECTED: map<A volatile, B> */
	          .simplified =
	              "map<A volatile, B, alloc<A const volatile, B>>"sv}},
	    {.input = "map<A, B, alloc<A, B>>"sv,
	     .expected =
	         {.str = "map<A, B, alloc<A, B>>"sv,
	          .repr =
	              "Expr[]{\"map\", <Expr[]{\"A\"}, Expr[]{\"B\"}, Expr[]{\"alloc\", <Expr[]{\"A\"}, Expr[]{\"B\"}>}>}"sv,
	          .simplified = "map<A, B, alloc<A, B>>"sv}},
	    {.input = "std::basic_string<char, std::char_traits<char>> const"sv,
	     .expected =
	         {.str = "std::basic_string<char, std::char_traits<char>> const"sv,
	          .repr =
	              "Expr[c]{\"std::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}>}"sv,
	          .simplified = "std::string const"sv}},
	    {.input = "std::basic_string<char, std::char_traits<char>>"sv,
	     .expected =
	         {.str = "std::basic_string<char, std::char_traits<char>>"sv,
	          .repr =
	              "Expr[]{\"std::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}>}"sv,
	          .simplified = "std::string"sv}},
	    {.input =
	         "std::vector<std::basic_string<char, std::char_traits<char>> volatile, std::allocator<std::basic_string<char, std::char_traits<char>> volatile>>"sv,
	     .expected =
	         {.str =
	              "std::vector<std::basic_string<char, std::char_traits<char>> volatile, std::allocator<std::basic_string<char, std::char_traits<char>> volatile>>"sv,
	          .repr =
	              "Expr[]{\"std::vector\", <Expr[v]{\"std::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}>}, Expr[]{\"std::allocator\", <Expr[v]{\"std::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}>}>}>}"sv,
	          .simplified = "std::vector<std::string volatile>"sv}},
	    {.input = "class_name<$1, $2>"sv,
	     .expected = {.str = "class_name<$1, $2>"sv,
	                  .repr =
	                      "Expr[]{\"class_name\", <Expr[]{$1}, Expr[]{$2}>}"sv,
	                  .simplified = "class_name<$1, $2>"sv}},
	    {.input = "class_name<item&, std::string const* const> &&"sv,
	     .expected =
	         {.str = "class_name<item&, std::string const* const>&&"sv,
	          .repr =
	              "Expr[&&]{\"class_name\", <Expr[&]{\"item\"}, Expr[c*c]{\"std::string\"}>}"sv,
	          .simplified = "class_name<item&, std::string const* const>&&"sv}},
	    {.input = "void foo(bar auto) &&"sv,
	     .expected =
	         {.str = "void foo(bar auto)&&"sv,
	          .repr =
	              "Expr[]{\"void\"} Expr[&&]{\"foo\", (Expr[]{\"bar\"} Expr[]{\"auto\"})}"sv,
	          .simplified = "void foo(bar auto)&&"sv}},
	    {.input = "std::basic_string<char, std::char_traits<char>> foo(int)"sv,
	     .expected =
	         {.str =
	              "std::basic_string<char, std::char_traits<char>> foo(int)"sv,
	          .repr =
	              "Expr[]{\"std::basic_string\", <Expr[]{\"char\"}, Expr[]{\"std::char_traits\", <Expr[]{\"char\"}>}>} Expr[]{\"foo\", (Expr[]{\"int\"})}"sv,
	          .simplified = "std::string foo(int)"sv}},
	    {.input = "void const* foo()"sv,
	     .expected = {.str = "void const* foo()"sv,
	                  .repr = "Expr[c*]{\"void\"} Expr[]{\"foo\", ()}"sv,
	                  .simplified = "void const* foo()"sv}},
	    {.input = "foo()::{lambda#1(auto:1, const auto:2&)}::operator()()"sv,
	     /*
	     THIS IS A BUG. EXPECTED:
	        foo()::{lambda#1(auto:1, auto:2 const&)}::operator()()
	     */
	     .expected =
	         {.str =
	              "foo()::{lambda#1(auto:1,  const auto:2 const&)}::operator()()"sv,
	          .repr =
	              "Expr[]{\"foo\", (), \"::{lambda#1\", (Expr[]{\"auto:1\"}, Expr[c]{} Expr[c&]{\"auto:2\"}), \"}::operator\", (), ()}"sv,
	          .simplified =
	              "foo()::{lambda#1(auto:1,  const auto:2 const&)}::operator()()"sv}},
	    {.input = "foo(int (&)[])"sv,
	     .expected =
	         {.str = "foo(int(&)[])"sv,
	          .repr =
	              "Expr[]{\"foo\", (Expr[]{\"int\", (Expr[&]{}), \"[]\"})}"sv,
	          .simplified = "foo(int(&)[])"sv}},
	    {.input = "auto operator<=>()"sv,
	     .expected = {.str = "auto operator<=>()"sv,
	                  .repr = "Expr[]{\"auto\"} Expr[]{\"operator<=>\", ()}"sv,
	                  .simplified = "auto operator<=>()"sv}},
	};
	INSTANTIATE_TEST_SUITE_P(tests, expression, ::testing::ValuesIn(tests));

	//
}  // namespace cxx_filt::testing
