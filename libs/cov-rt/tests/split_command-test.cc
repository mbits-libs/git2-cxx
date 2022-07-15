// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/app/split_command.hh>

namespace cov::app::testing {
	using namespace ::std::literals;

	struct split_test {
		std::string_view name;
		std::string_view input;
		std::vector<std::string> expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                split_test const& test) {
			return out << test.name;
		}
	};

	class split_command : public ::testing::TestWithParam<split_test> {};

	TEST_P(split_command, report) {
		auto const& [_, input, expected] = GetParam();

		auto actual = app::split_command(input);
		ASSERT_EQ(expected, actual);
	}

	static const split_test tests[] = {
	    {
	        "empty"sv,
	        R"()"sv,
	        {},
	    },
	    {
	        "simple"sv,
	        R"(  before and after )"sv,
	        {"before"s, "and"s, "after"s},
	    },
	    {
	        "escape a space"sv,
	        R"(before and\ after)"sv,
	        {"before"s, "and after"s},
	    },
	    {
	        "escape something else"sv,
	        R"(before a\nd after)"sv,
	        {"before"s, "and"s, "after"s},
	    },
	    {
	        "simple quoted spaces"sv,
	        R"("quoted text" 'quoted text')"sv,
	        {"quoted text"s, "quoted text"s},
	    },
	    {
	        "escaping a space inside quotes"sv,
	        R"("quoted\ text" 'quoted\ text')"sv,
	        {"quoted text"s, "quoted\\ text"s},
	    },
	    {
	        "escaping a \" inside quotes"sv,
	        R"("quoted\"text" 'quoted\"text')"sv,
	        {"quoted\"text"s, "quoted\\\"text"s},
	    },
	    {
	        "double, half-open"sv,
	        R"("broken" "but working)"sv,
	        {"broken"s, "but working"s},
	    },
	    {
	        "single, half-open"sv,
	        R"('broken' 'but working)"sv,
	        {"broken"s, "but working"s},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, split_command, ::testing::ValuesIn(tests));
}  // namespace cov::app::testing
