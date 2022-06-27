// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/io/strings.hh>

namespace cov::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	struct strings_test {
		std::string_view title;
		std::string_view builder{};
		std::string_view block{};
		std::vector<std::string_view> strings{};
		std::vector<size_t> offsets{};

		friend std::ostream& operator<<(std::ostream& out,
		                                strings_test const& param) {
			return out << param.title;
		}
	};

	class strings : public TestWithParam<strings_test> {};

	TEST_P(strings, build) {
		auto [title, expected, _, strings, __] = GetParam();

		io::strings_builder builder{};
		for (auto str : strings)
			builder.insert(str);

		auto const result = builder.build();
		auto const actual = std::string_view{result.data(), result.size()};
		ASSERT_EQ(expected, actual);
		ASSERT_TRUE(result.is_valid());
	}

	TEST_P(strings, block) {
		auto [title, _, expected, strings, __] = GetParam();

		io::strings_block block{};
		block.reserve_offsets(block.size());
		for (auto str : strings) {
			auto const offset = block.locate(str);
			if (offset > block.size()) block.append(str);
		}

		// deliberately no fill32...

		auto const actual = std::string_view{block.data(), block.size()};
		ASSERT_EQ(expected, actual);
		ASSERT_TRUE(block.is_valid());
	}

	TEST_P(strings, view) {
		auto [title, _, data, strings, offsets] = GetParam();
		ASSERT_GE(offsets.size(), strings.size());

		io::strings_view view{};
		view.data.insert(view.data.end(), data.begin(), data.end());
		view.resync();
		auto const count = strings.size();
		for (auto index = 0u; index < count; ++index) {
			auto const offset = offsets[index];
			ASSERT_TRUE(view.is_valid(offset))
			    << "Index: " << index << "\nOffset: " << offset
			    << "\nString: " << strings[index];
			ASSERT_EQ(strings[index], view.at(offset))
			    << "Index: " << index << "\nString: " << strings[index];
		}
	}

	TEST_P(strings, copied_view) {
		auto [title, _, data, strings, offsets] = GetParam();
		ASSERT_GE(offsets.size(), strings.size());

		io::strings_view view{};
		{
			io::strings_view orig{};
			orig.data.insert(orig.data.end(), data.begin(), data.end());
			orig.resync();
			auto moved_to = std::move(orig);
			auto copied_to = moved_to;

			io::strings_view moved_into{};
			moved_into = std::move(moved_to);
			view = moved_into;
		}
		auto const count = strings.size();
		for (auto index = 0u; index < count; ++index) {
			auto const offset = offsets[index];
			ASSERT_TRUE(view.is_valid(offset))
			    << "Index: " << index << "\nOffset: " << offset
			    << "\nString: " << strings[index];
			ASSERT_EQ(strings[index], view.at(offset))
			    << "Index: " << index << "\nString: " << strings[index];
		}
	}

	static strings_test const list[] = {
	    {"empty"sv},
	    {
	        "short string"sv,
	        "0123456789\0"
	        "\0"sv,
	        "0123456789\0"sv,
	        {"0123456789"sv},
	        {0u},
	    },
	    {
	        "10 strings"sv,
	        "string 0\0string 1\0string 2\0string 3\0string 4\0"
	        "string 5\0string 6\0string 7\0string 8\0string 9\0"
	        "\0\0"sv,
	        "string 0\0string 1\0string 2\0string 3\0string 4\0string "
	        "5\0string 6\0string 7\0string 8\0string 9\0"
	        ""sv,
	        {"string 0"sv, "string 1"sv, "string 2"sv, "string 3"sv,
	         "string 4"sv, "string 5"sv, "string 6"sv, "string 7"sv,
	         "string 8"sv, "string 9"sv},
	        {0u, 9u, 18u, 27u, 36u, 45u, 54u, 63u, 72u, 81u, 90u},
	    },
	    {
	        "duplicated"sv,
	        "first string\0fourth\0second string\0third one\0"sv,
	        "first string\0second string\0third one\0fourth\0"sv,
	        {"first string"sv, "second string"sv, "third one"sv,
	         "second string"sv, "fourth"sv, "second string"sv,
	         "first string"sv},
	        {0u, 13u, 27u, 13u, 37u, 13u, 0u},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(list, strings, ValuesIn(list));
}  // namespace cov::testing
