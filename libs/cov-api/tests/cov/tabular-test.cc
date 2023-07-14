// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/format.hh>
#include <cov/tabular.hh>

using namespace std::literals;

namespace cov::testing {
	using placeholder::color;
	inline std::string colorize(color clr) {
		return formatter::shell_colorize(clr, nullptr);
	}
	inline std::string esc_color(color clr, std::string_view label) {
		return fmt::format("{}{}{}", colorize(clr), label,
		                   colorize(color::reset));
	}

	TEST(tabular, empty) {
		data_table table{};
		ASSERT_EQ(R"(+
+
)"s,
		          table.format());
		table.print(stdout);
	}

	TEST(tabular, colors) {
		data_table table{
		    .header = {{"1"s, col_span<1>{}, '<'},
		               {"2"s, col_span<1>{}},
		               {"3"s, col_span<1>{}, '>'}},
		};
		table.emplace({"ABC"s, "123"s, "xyz"s});
		table.emplace({
		    esc_color(color::red, "A"sv),
		    esc_color(color::green, "B"sv),
		    esc_color(color::blue, "C"sv),
		});
		ASSERT_EQ(
		    "+-----+-----+-----+\n"
		    "| 1   |  2  |   3 |\n"
		    "+-----+-----+-----+\n"
		    "| ABC | 123 | xyz |\n"
		    "|   \x1B[31mA\x1B[m |   \x1B[32mB\x1B[m |   \x1B[34mC\x1B[m |\n"
		    "+-----+-----+-----+\n"s,
		    table.format());
		table.print(stdout);
	}

	TEST(tabular, listing) {
		data_table table{
		    .header = {{"NAME"s, '<'}, "STATS"s},
		};
		table.emplace({" "s, "main.cc"s, "1234"s, "+5"s});
		table.emplace({"*"s, "src/proxy.cc"s, "21"s, "-1"s});
		table.emplace({" "s, "module.cc"s, "15"s, ""s});
		table.emplace({" "s, "banana \xF0\x9F\x8D\x8C"s, "10000"s, "+10000"s});
		table.emplace({" "s, "filename 5"s, "1234"s, ""s});
		table.emplace({" "s, "TOTAL"s, "???"s, "+10004"s}, false);
		ASSERT_EQ(
		    "+----------------+--------------+\n"
		    "| NAME           |    STATS     |\n"
		    "+----------------+--------------+\n"
		    "|   main.cc      |  1234 +5     |\n"
		    "| * src/proxy.cc |    21 -1     |\n"
		    "|   module.cc    |    15        |\n"
		    "|   banana \xF0\x9F\x8D\x8C    | 10000 +10000 |\n"
		    "|   filename 5   |  1234        |\n"
		    "+----------------+--------------+\n"
		    "|   TOTAL        |   ??? +10004 |\n"
		    "+----------------+--------------+\n",
		    table.format());
		table.print(stdout);
	}

	TEST(tabular, unicode) {
		data_table table{
		    .header = {"A"s},
		};
		table.emplace({"A"s, "xyzaecxyz"s});
		table.emplace({"U"s, "xy\xC5\xBA\xC4\x85\xC3\xAA\xC3\xA7xyz"s});
		ASSERT_EQ(
		    "+-------------+\n"
		    "|      A      |\n"
		    "+-------------+\n"
		    "| A xyzaecxyz |\n"
		    "| U xy\xC5\xBA\xC4\x85\xC3\xAA\xC3\xA7xyz |\n"
		    "+-------------+\n"s,
		    table.format());
		table.print(stdout);
	}
}  // namespace cov::testing
