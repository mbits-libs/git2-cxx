// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <c++filt/json.hh>
#include <string_view>

namespace cxx_filt::testing {
	using namespace ::std::literals;

	struct json_test {
		std::string_view name;
		std::string_view input;
		std::map<std::string_view, std::string_view> expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                json_test const& test) {
			return out << test.name;
		}
	};

	class json : public ::testing::TestWithParam<json_test> {};

	TEST_P(json, load) {
		auto const& [_, input, expected] = GetParam();
		Replacements actual{};
		append_replacements(input, actual);

		ASSERT_EQ(expected.size(), actual.size());

		for (auto const& [actual_from, actual_to] : actual) {
			auto expected_it = expected.find(actual_from.str());
			if (expected_it == expected.end()) {
				ASSERT_TRUE(false)
				    << "The '" << actual_from.str()
				    << "' expression was not found in list of expected.";
			}
			ASSERT_EQ(expected_it->first, actual_from.str());
			ASSERT_EQ(expected_it->second, actual_to.str());
		}
	}

	static json_test const tests[] = {
	    {
	        .name = "direct list"sv,
	        .input = R"({"a": "b", "b": "c", "c": "d"})"sv,
	        .expected =
	            {
	                {"a"sv, "b"sv},
	                {"b"sv, "c"sv},
	                {"c"sv, "d"sv},
	            },
	    },
	    {
	        .name = "embedded objects"sv,
	        .input = R"({
    "a": {"a": "b", "b": "c", "c": "d"},
    "b": {"d": "e", "e": "f", "f": "g"}
})"sv,
	        .expected =
	            {
	                {"a"sv, "b"sv},
	                {"b"sv, "c"sv},
	                {"c"sv, "d"sv},
	                {"d"sv, "e"sv},
	                {"e"sv, "f"sv},
	                {"f"sv, "g"sv},
	            },
	    },
	    {
	        .name = "embedded in array"sv,
	        .input =
	            R"([{"a": "b", "b": "c", "c": "d"}, {"d": "e", "e": "f", "f": "g"}])"sv,
	        .expected =
	            {
	                {"a"sv, "b"sv},
	                {"b"sv, "c"sv},
	                {"c"sv, "d"sv},
	                {"d"sv, "e"sv},
	                {"e"sv, "f"sv},
	                {"f"sv, "g"sv},
	            },
	    },
	    {
	        .name = "function in key"sv,
	        .input = R"x({"a": "b", "b": "c", "void c()": "d"})x"sv,
	        .expected =
	            {
	                {"a"sv, "b"sv},
	                {"b"sv, "c"sv},
	            },
	    },
	    {
	        .name = "function in replacement"sv,
	        .input = R"x({"a": "b", "b": "c", "c": "void d()"})x"sv,
	        .expected =
	            {
	                {"a"sv, "b"sv},
	                {"b"sv, "c"sv},
	            },
	    },
	    {
	        .name = "empty replacement"sv,
	        .input = R"x({"a": "b", "b": "c", "c": ""})x"sv,
	        .expected =
	            {
	                {"a"sv, "b"sv},
	                {"b"sv, "c"sv},
	            },
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, json, ::testing::ValuesIn(tests));
}  // namespace cxx_filt::testing
