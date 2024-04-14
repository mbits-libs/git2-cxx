// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <map>
#include "native/str.hh"
#include "strip/strip.hh"

using namespace std::literals;

namespace cov::app::strip {
	void PrintTo(const excl_block& block, std::ostream* os) {
		*os << '{' << block.start << ',' << block.end << '}';
	}

	void PrintTo(const exclude_report_line& line, std::ostream* os) {
		*os << '{' << line.line << ", \"" << line.counter << "\"s, \""
		    << line.text << "\"s}";
	}
}  // namespace cov::app::strip

namespace cov::app::strip::testing {
	struct strip_test {
		std::string_view name;
		std::string_view text;
		std::vector<std::string_view> markers{};
		struct {
			exclude_result blocks;
			std::vector<exclude_report_line> lines;
			std::map<unsigned, unsigned> coverage;
			unsigned erased_lines;
		} expected;
		friend std::ostream& operator<<(std::ostream& out,
		                                strip_test const& tc) {
			return out << tc.name;
		}
	};

	class strip : public ::testing::TestWithParam<strip_test> {};

	TEST_P(strip, find_blocks) {
		auto const& [_, text, markers, expected] = GetParam();
		auto const& [expected_blocks, expected_blanks] = expected.blocks;
		auto const& [actual_blocks, actual_blanks] =
		    find_blocks("a-file", markers, text);

		ASSERT_EQ(expected_blanks, actual_blanks);
		ASSERT_EQ(expected_blocks, actual_blocks);
	}

	TEST_P(strip, find_lines) {
		auto const& [_, text, markers, expected] = GetParam();
		json::map coverage{};
		for (unsigned line = 1; line < 15; ++line) {
			if (line % 3 == 0) continue;
			auto const key = fmt::format("{}", line);
			coverage.set(to_u8s(key), line % 2 ? 0 : 1);
		}
		auto const actual = find_lines(coverage, expected.blocks.first, text);

		ASSERT_EQ(expected.lines, actual);
	}

	TEST_P(strip, erase_lines) {
		auto const& [_, text, markers, expected] = GetParam();
		json::map coverage{};
		for (unsigned line = 1; line < 15; ++line) {
			if (line % 3 == 0) continue;
			auto const key = fmt::format("{}", line);
			coverage.set(to_u8s(key), line % 2 ? 0 : 1);
		}
		auto const actual_counter = erase_lines(coverage, expected.blocks.first,
		                                        expected.blocks.second);

		std::map<std::string, unsigned> expected_coverage{}, actual_coverage{};
		for (auto const [key, value] : expected.coverage) {
			expected_coverage[fmt::format("{}", key)] = value;
		}
		for (auto const& [key, value] : coverage.items()) {
			auto const val = cast<long long>(value);
			actual_coverage[from_u8s(key)] =
			    val ? static_cast<unsigned>(*val)
			        : std::numeric_limits<unsigned>::max();
		}

		ASSERT_EQ(expected_coverage, actual_coverage)
		    << ".erased_lines = " << actual_counter;
		ASSERT_EQ(expected.erased_lines, actual_counter);
	}

	static const strip_test tests[] = {
	    {
	        .name = "no-blocks"sv,
	        .text = R"()"sv,
	        .expected = {.blocks = {{}, {1}},
	                     .lines = {},
	                     .coverage = {{2, 1},
	                                  {4, 1},
	                                  {5, 0},
	                                  {7, 0},
	                                  {8, 1},
	                                  {10, 1},
	                                  {11, 0},
	                                  {13, 0},
	                                  {14, 1}},
	                     .erased_lines = 1},
	    },
	    {
	        .name = "single line"sv,
	        .text = R"(text
text
text // GCOV_EXCL_LINE
text

text
text // LCOV_EXCL_LINE
text

text
text // GRCOV_EXCL_LINE
text
)"sv,
	        .expected =
	            {.blocks = {{{3, 3}, {7, 7}, {11, 11}}, {5, 9, 13}},
	             .lines = {{3, ""s, "text // GCOV_EXCL_LINE"s},
	                       {7, "0"s, "text // LCOV_EXCL_LINE"s},
	                       {11, "0"s, "text // GRCOV_EXCL_LINE"s}},
	             .coverage = {{1, 0}, {2, 1}, {4, 1}, {8, 1}, {10, 1}, {14, 1}},
	             .erased_lines = 4},
	    },
	    {
	        .name = "blocks"sv,
	        .text = R"(text // GCOV_EXCL_START
text
text
text // GCOV_EXCL_STOP

text // LCOV_EXCL_START
text
text // LCOV_EXCL_STOP

text // GRCOV_EXCL_START
text
text // GRCOV_EXCL_STOP
)"sv,
	        .expected = {.blocks = {{{1, 4}, {6, 8}, {10, 12}}, {5, 9, 13}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START"s},
	                               {2, "1"s, "text"s},
	                               {3, ""s, "text"s},
	                               {4, "1"s, "text // GCOV_EXCL_STOP"s},
	                               {6, ""s, "text // LCOV_EXCL_START"s},
	                               {7, "0"s, "text"s},
	                               {8, "1"s, "text // LCOV_EXCL_STOP"s},
	                               {10, "1"s, "text // GRCOV_EXCL_START"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GRCOV_EXCL_STOP"s}},
	                     .coverage = {{14, 1}},
	                     .erased_lines = 9},
	    },
	    {
	        .name = "mismatched start-stop"sv,
	        .text = R"(text // GCOV_EXCL_START
text
text
text // LCOV_EXCL_STOP

text // LCOV_EXCL_START
text
text // GRCOV_EXCL_STOP

text // GRCOV_EXCL_START
text
text // GCOV_EXCL_STOP
)"sv,
	        .expected = {.blocks = {{{1, 4}, {6, 8}, {10, 12}}, {5, 9, 13}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START"s},
	                               {2, "1"s, "text"s},
	                               {3, ""s, "text"s},
	                               {4, "1"s, "text // LCOV_EXCL_STOP"s},
	                               {6, ""s, "text // LCOV_EXCL_START"s},
	                               {7, "0"s, "text"s},
	                               {8, "1"s, "text // GRCOV_EXCL_STOP"s},
	                               {10, "1"s, "text // GRCOV_EXCL_START"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GCOV_EXCL_STOP"s}},
	                     .coverage = {{14, 1}},
	                     .erased_lines = 9},
	    },
	    {
	        .name = "mismatched start-end"sv,
	        .text = R"(text // GCOV_EXCL_START
text
text
text // GCOV_EXCL_STOP

text // LCOV_EXCL_START
text
text // LCOV_EXCL_END

text // GRCOV_EXCL_START
text
text // GRCOV_EXCL_STOP
)"sv,
	        .expected = {.blocks = {{{1, 4}, {10, 12}}, {5, 9, 13}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START"s},
	                               {2, "1"s, "text"s},
	                               {3, ""s, "text"s},
	                               {4, "1"s, "text // GCOV_EXCL_STOP"s},
	                               {10, "1"s, "text // GRCOV_EXCL_START"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GRCOV_EXCL_STOP"s}},
	                     .coverage = {{7, 0}, {8, 1}, {14, 1}},
	                     .erased_lines = 7},
	    },
	    {
	        .name = "mismatched double"sv,
	        .text = R"(text // GCOV_EXCL_START
text
text
text // GCOV_EXCL_STOP

text // LCOV_EXCL_START
text
text // LCOV_EXCL_SOMETHING_ELSE

text // GRCOV_EXCL_START
text
text // GRCOV_EXCL_STOP
)"sv,
	        .expected = {.blocks = {{{1, 4}, {10, 12}}, {5, 9, 13}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START"s},
	                               {2, "1"s, "text"s},
	                               {3, ""s, "text"s},
	                               {4, "1"s, "text // GCOV_EXCL_STOP"s},
	                               {10, "1"s, "text // GRCOV_EXCL_START"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GRCOV_EXCL_STOP"s}},
	                     .coverage = {{7, 0}, {8, 1}, {14, 1}},
	                     .erased_lines = 7},
	    },
	    {
	        .name = "mismatched ending"sv,
	        .text = R"(text // GCOV_EXCL_START
text
text
text // GCOV_EXCL_STOP

text // LCOV_EXCL_START
text
text

text
text
text
)"sv,
	        .expected =
	            {.blocks = {{{1, 4}}, {5, 9, 13}},
	             .lines = {{1, "0"s, "text // GCOV_EXCL_START"s},
	                       {2, "1"s, "text"s},
	                       {3, ""s, "text"s},
	                       {4, "1"s, "text // GCOV_EXCL_STOP"s}},
	             .coverage = {{7, 0}, {8, 1}, {10, 1}, {11, 0}, {14, 1}},
	             .erased_lines = 5},
	    },
	    {
	        .name = "mismatched with single lines"sv,
	        .text = R"(text // GCOV_EXCL_START
text // GCOV_EXCL_LINE
text
text // GCOV_EXCL_STOP

text
text // GCOV_EXCL_LINE
text

text // GCOV_EXCL_START
text // GCOV_EXCL_LINE
text)"sv,
	        .expected = {.blocks = {{{1, 4}, {7, 7}, {11, 11}}, {5, 9}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START"s},
	                               {2, "1"s, "text // GCOV_EXCL_LINE"s},
	                               {3, ""s, "text"s},
	                               {4, "1"s, "text // GCOV_EXCL_STOP"s},
	                               {7, "0"s, "text // GCOV_EXCL_LINE"s},
	                               {11, "0"s, "text // GCOV_EXCL_LINE"s}},
	                     .coverage = {{8, 1}, {10, 1}, {13, 0}, {14, 1}},
	                     .erased_lines = 6},
	    },
	};

	static constexpr auto tagged_text =
	    R"(text // GCOV_EXCL_START[MARK1]
text
text // GCOV_EXCL_STOP
text // GCOV_EXCL_START[MARK2]
text
text
text // GCOV_EXCL_STOP
text // GCOV_EXCL_START[MARK1, MARK2]
text
text
text
text // GCOV_EXCL_STOP
text
text
text // GCOV_EXCL_START[MARK3 , UNUSED ]
text
text
text
text // GCOV_EXCL_STOP
text
text
text // GCOV_EXCL_START[Mark1]
text
text
text
text
text // GCOV_EXCL_STOP
text
text)"sv;

	static const strip_test markers[] = {
	    {
	        .name = "no tags"sv,
	        .text = tagged_text,
	        .markers = {},
	        .expected = {.blocks = {{}, {}},
	                     .lines = {},
	                     .coverage = {{1, 0},
	                                  {2, 1},
	                                  {4, 1},
	                                  {5, 0},
	                                  {7, 0},
	                                  {8, 1},
	                                  {10, 1},
	                                  {11, 0},
	                                  {13, 0},
	                                  {14, 1}},
	                     .erased_lines = 0},
	    },
	    {
	        .name = "mark1"sv,
	        .text = tagged_text,
	        .markers = {"mark1"sv},
	        .expected = {.blocks = {{{1, 3}, {8, 12}, {22, 27}}, {}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START[MARK1]"s},
	                               {2, "1"s, "text"s},
	                               {3, ""s, "text // GCOV_EXCL_STOP"s},
	                               {8, "1"s,
	                                "text // GCOV_EXCL_START[MARK1, MARK2]"s},
	                               {9, ""s, "text"s},
	                               {10, "1"s, "text"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GCOV_EXCL_STOP"s},
	                               {22, ""s, "text // GCOV_EXCL_START[Mark1]"s},
	                               {23, ""s, "text"s},
	                               {24, ""s, "text"s},
	                               {25, ""s, "text"s},
	                               {26, ""s, "text"s},
	                               {27, ""s, "text // GCOV_EXCL_STOP"s}},
	                     .coverage = {{4, 1}, {5, 0}, {7, 0}, {13, 0}, {14, 1}},
	                     .erased_lines = 5},
	    },
	    {
	        .name = "mark2"sv,
	        .text = tagged_text,
	        .markers = {"mark2"sv},
	        .expected = {.blocks = {{{4, 12}}, {}},
	                     .lines = {{4, "1"s, "text // GCOV_EXCL_START[MARK2]"s},
	                               {5, "0"s, "text"s},
	                               {6, ""s, "text"s},
	                               {7, "0"s, "text // GCOV_EXCL_STOP"s},
	                               {8, "1"s,
	                                "text // GCOV_EXCL_START[MARK1, MARK2]"s},
	                               {9, ""s, "text"s},
	                               {10, "1"s, "text"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GCOV_EXCL_STOP"s}},
	                     .coverage = {{1, 0}, {2, 1}, {13, 0}, {14, 1}},
	                     .erased_lines = 6},
	    },
	    {
	        .name = "mark1 + mark2"sv,
	        .text = tagged_text,
	        .markers = {"mark1"sv, "mark2"sv},
	        .expected = {.blocks = {{{1, 12}, {22, 27}}, {}},
	                     .lines = {{1, "0"s, "text // GCOV_EXCL_START[MARK1]"s},
	                               {2, "1"s, "text"s},
	                               {3, ""s, "text // GCOV_EXCL_STOP"s},
	                               {4, "1"s, "text // GCOV_EXCL_START[MARK2]"s},
	                               {5, "0"s, "text"s},
	                               {6, ""s, "text"s},
	                               {7, "0"s, "text // GCOV_EXCL_STOP"s},
	                               {8, "1"s,
	                                "text // GCOV_EXCL_START[MARK1, MARK2]"s},
	                               {9, ""s, "text"s},
	                               {10, "1"s, "text"s},
	                               {11, "0"s, "text"s},
	                               {12, ""s, "text // GCOV_EXCL_STOP"s},
	                               {22, ""s, "text // GCOV_EXCL_START[Mark1]"s},
	                               {23, ""s, "text"s},
	                               {24, ""s, "text"s},
	                               {25, ""s, "text"s},
	                               {26, ""s, "text"s},
	                               {27, ""s, "text // GCOV_EXCL_STOP"s}},
	                     .coverage = {{13, 0}, {14, 1}},
	                     .erased_lines = 8},
	    },
	    {
	        .name = "mark3"sv,
	        .text = tagged_text,
	        .markers = {"mark3"sv},
	        .expected =
	            {.blocks = {{{15, 19}}, {}},
	             .lines = {{15, ""s,
	                        "text // GCOV_EXCL_START[MARK3 , UNUSED ]"s},
	                       {16, ""s, "text"s},
	                       {17, ""s, "text"s},
	                       {18, ""s, "text"s},
	                       {19, ""s, "text // GCOV_EXCL_STOP"s}},
	             .coverage = {{1, 0},
	                          {2, 1},
	                          {4, 1},
	                          {5, 0},
	                          {7, 0},
	                          {8, 1},
	                          {10, 1},
	                          {11, 0},
	                          {13, 0},
	                          {14, 1}},
	             .erased_lines = 0},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(test, strip, ::testing::ValuesIn(tests));
	INSTANTIATE_TEST_SUITE_P(marker, strip, ::testing::ValuesIn(markers));
}  // namespace cov::app::strip::testing
