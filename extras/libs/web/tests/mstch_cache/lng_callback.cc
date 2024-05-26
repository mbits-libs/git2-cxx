// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)
//
// IT IS NATURAL FOR TEST IN HERE TO FAIL FROM TIME TO TIME. IF THEY SWITCH FROM
// SUCCEEDING TO FAILING, A TEST NEEDS TO BE UPDATED TO REFLECT CHANGE IN THE
// REST OF THE CODE.

#include "common.hh"

namespace cov::app::web::testing {
	struct lng_test {
		std::string_view key;
		std::string_view expected;

		friend std::ostream& operator<<(std::ostream& out,
		                                lng_test const& test) {
			return out << test.key;
		}
	};

	static constexpr lng_test lng_tests[] = {
#define X(LNG) {"LNG_" #LNG ""sv, #LNG " (1)"sv},
	    MSTCH_LNG(X)
#undef X
	};

	class lng_callback : public ::testing::TestWithParam<lng_test> {};
	INSTANTIATE_TEST_SUITE_P(string,
	                         lng_callback,
	                         ::testing::ValuesIn(lng_tests));

	TEST(lng_callback, keys) {
		std::set<std::string_view> expected{};
		for (auto const& test : lng_tests) {
			expected.insert(test.key);
		}

		auto const lng_callback = tests::lng_callback();
		auto const keys = lng_callback->debug_all_keys();

		ASSERT_EQ(expected.size(), keys.size());
		for (auto const& key : keys) {
			ASSERT_EQ(1, expected.count(key)) << "key: " << key;
		}
	}

	TEST(lng_callback, unknown) {
		auto const lng_callback = tests::lng_callback();
		auto const key = "LNG_SOMETHING_SOMETHING"s;
		auto const actual_node = lng_callback->at(key);
		ASSERT_TRUE(std::holds_alternative<std::string>(actual_node));
		auto const& actual = std::get<std::string>(actual_node);

		ASSERT_EQ("\xC2\xABLNG_SOMETHING_SOMETHING\xC2\xBB", actual);
	}

	TEST_P(lng_callback, tr) {
		auto const& [name, expected] = GetParam();
		auto const lng_callback = tests::lng_callback();
		auto const key = std::string(name);
		ASSERT_TRUE(lng_callback->has(key));
		for (unsigned repeat = 1; repeat < 11; ++repeat) {
			auto const actual_node = lng_callback->at(key);
			ASSERT_TRUE(std::holds_alternative<std::string>(actual_node));
			auto const& actual = std::get<std::string>(actual_node);

			ASSERT_EQ(expected, actual) << "repeat: " << repeat;
		}
	}
}  // namespace cov::app::web::testing
